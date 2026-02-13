// Protobrain V2 for RP2040

// OSD chip might not work if powered by long/shitty USB cable, requires 5V
// On startup all GPIOs are in hi-z with a pull-down

// TODO: Current sense, auto brightness scale down if above max current
// TOOD: Interactive console to set values, save/load to txt file

// NOTE: The old MAX7456 driver assumes that the chip was loaded with a modified charset to match the standard ASCII table,
// this won't work with a brand new MAX7456.

// ADC values for automatic brightness adjustment (GL5528 photoresistor + 10k pulldown)

// Note that the perceived brightness of each channel is not equal:
//   Red 405   Green 690   Blue 190
// So if we normalise around the capability of the blue channel, for equal
// brightness the channels should be scaled like:
//   Red 0.47  Green 0.28  Blue 1.0
// I won't do the brightness equalisation now, though.

#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#include <pico/assert.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include "adc_sensors.h"
#include "animation/anim.h"
#include "leds/led_brightness.h"
#include "leds/leds.h"
#include "osd.h"
#include "remote/remote.h"
#include "work_queue.h"

#ifdef USE_SD_CARD
#include "sd_config.h"
#endif

/***********************
 * Defines and types
 ***********************/

/** Read the ADC sensors at 100Hz, producing average values at 10Hz. */
#define ADC_READ_PERIOD_MS 10
/** Update the OSD every 0.5 second */
#define OSD_UPDATE_PERIOD_MS 500
/** Play a random animation every 10 seconds. */
#define RANDOM_ANIMATION_PERIOD_MS (10 * 1000)

#define REPEATING_TIMER_CONTINUE true
#define ALARM_STOP 0
#define ALARM_RESCHEDULE_AFTER_MS(ms) (-(ms))

typedef enum
{
    WORK_ITEM_ANIMATE_FACE_FRAME,
    WORK_ITEM_REQUEST_RANDOM_ANIMATION,
    WORK_ITEM_READ_ADC_SENSORS,
    WORK_ITEM_UPDATE_OSD,
} main_work_item_command_t;
static_assert(sizeof(main_work_item_command_t) <= sizeof(work_command_t), "too big");

/***********************
 * Variables
 ***********************/

static repeating_timer_t face_animation_timer;
static repeating_timer_t adc_read_timer;
static repeating_timer_t osd_update_timer;

/** We use this as a non-repeating timer. */
static alarm_id_t random_animation_timer;
static atomic_bool is_random_animation_timer_running;
static bool want_random_animation;

static uint16_t animation_period_ms;
static ws2812b_led_value_t logo_colour;

/***********************
 * Function prototypes
 ***********************/

static void do_work(work_item_t work);
static bool face_animation_callback(repeating_timer_t *timer);
static bool adc_read_callback(repeating_timer_t *timer);
static bool osd_update_callback(repeating_timer_t *timer);
static int64_t random_animation_callback(alarm_id_t id, void *user_data);

/***********************
 * Public functions
 ***********************/

int main(void)
{
    hard_assert(stdio_init_all());

    work_queue_init();

#ifdef USE_SD_CARD
    sd_config_init();
#endif

    adc_sensors_init();
    led_brightness_init(adc_sensors_get_averages().brightness);

    leds_init();
    sleep_ms(1);

    /* For now, just set the cheek and body logos to a fixed colour. */
    logo_colour = (ws2812b_led_value_t){.r = 0, .g = 0, .b = led_brightness_get_logo_value()};
    leds_set_channel_to_colour(LED_CHANNEL_CHEEK, logo_colour, false);
    leds_set_channel_to_colour(LED_CHANNEL_BODY0, logo_colour, false);
    leds_set_channel_to_colour(LED_CHANNEL_BODY1, logo_colour, false);

    osd_init();
    remote_init();

    /* Start the boot animation. */
    hard_assert(animationInit());
    animation_period_ms = startAnimation(BOOT_ANIMATION);
    hard_assert(animation_period_ms != 0);

    /* Start the timers last, so we don't accumulate lots of work during any slow parts of init. */
    hard_assert(
        add_repeating_timer_ms(
            animation_period_ms, face_animation_callback, NULL, &face_animation_timer));
    hard_assert(
        add_repeating_timer_ms(
            ADC_READ_PERIOD_MS, adc_read_callback, NULL, &adc_read_timer));
    hard_assert(
        add_repeating_timer_ms(
            OSD_UPDATE_PERIOD_MS, osd_update_callback, NULL, &osd_update_timer));

    while (true)
    {
        work_item_t work = work_queue_remove_blocking();
        switch (work.destination)
        {
        case WORK_MODULE_MAIN:
            do_work(work);
            break;

        case WORK_MODULE_REMOTE:
            remote_handle_work(work);
            break;

        default:
            /* Unrecognised destination, something has gone wrong. */
            hard_assert(false);
            break;
        }
    }
}

/***********************
 * Private functions
 ***********************/

static void do_work(work_item_t work)
{
    main_work_item_command_t command = (main_work_item_command_t)work.command;
    switch (command)
    {
    case WORK_ITEM_ANIMATE_FACE_FRAME:
    {
        bool finished = updateAnimation();
        if (finished)
        {
            cancel_repeating_timer(&face_animation_timer);

            uint8_t next_animation = DEFAULT_ANIMATION;
            bool starting_random = false;

            if (want_random_animation)
            {
                starting_random = true;
                want_random_animation = false;

                switch (rand() % 4)
                {
                case 0:
                    next_animation = RANDOM_ANIMATION_1;
                    break;
                case 1:
                    next_animation = RANDOM_ANIMATION_2;
                    break;
                case 2:
                    next_animation = RANDOM_ANIMATION_3;
                    break;
                default:
                    /* Intentionally have a chance to pick the default animation. */
                    next_animation = DEFAULT_ANIMATION;
                    break;
                }
            }

            animation_period_ms = startAnimation(next_animation);
            hard_assert(
                add_repeating_timer_ms(
                    animation_period_ms, face_animation_callback, NULL, &face_animation_timer));

            /* We start the random animation timer if it's not already running and we didn't
             * just start a random animation. In practice this means we start the timer at the
             * end of the boot animation, and then at the end of each random animation.
             * We do it like this so the time between random animations is correct if any of
             * the animations are long (which they are). */
            if (!starting_random && !is_random_animation_timer_running)
            {
                random_animation_timer = add_alarm_in_ms(
                    RANDOM_ANIMATION_PERIOD_MS, random_animation_callback, NULL, true);
                hard_assert(random_animation_timer > 0);
                is_random_animation_timer_running = true;
            }
        }
    }
    break;

    case WORK_ITEM_REQUEST_RANDOM_ANIMATION:
        want_random_animation = true;
        break;

    case WORK_ITEM_READ_ADC_SENSORS:
    {
        bool averages_updated = adc_sensors_read();
        if (averages_updated)
        {
            led_brightness_update(adc_sensors_get_averages().brightness);

            /* Logo auto-brightness adjustment. Still a fixed colour. */
            logo_colour.b = led_brightness_get_logo_value();
            leds_set_channel_to_colour(LED_CHANNEL_CHEEK, logo_colour, false);
            leds_set_channel_to_colour(LED_CHANNEL_BODY0, logo_colour, false);
            leds_set_channel_to_colour(LED_CHANNEL_BODY1, logo_colour, false);
        }
    }
    break;

    case WORK_ITEM_UPDATE_OSD:
    {
        uint32_t ms_since_boot = to_ms_since_boot(get_absolute_time());
        const uint8_t fake_remote_data[] = {0, 0};
        osd_update(
            adc_sensors_get_averages().battery_v,
            ms_since_boot,
            animation_get_current_name(),
            fake_remote_data);
    }
    break;

    default:
    {
        /* Unrecognised work, something has gone wrong. */
        hard_assert(false);
        break;
    }
    }
}

static bool face_animation_callback(repeating_timer_t *timer)
{
    (void)timer;
    work_item_t work = {
        .destination = WORK_MODULE_MAIN,
        .command = WORK_ITEM_ANIMATE_FACE_FRAME,
    };
    work_queue_try_add(work);
    /* Assume we want to draw more frames. If the work queue is full this just results in a
     * temporarily lower framerate. */
    return REPEATING_TIMER_CONTINUE;
}

static int64_t random_animation_callback(alarm_id_t id, void *user_data)
{
    (void)id;
    (void)user_data;
    work_item_t work = {
        .destination = WORK_MODULE_MAIN,
        .command = WORK_ITEM_REQUEST_RANDOM_ANIMATION,
    };
    if (work_queue_try_add(work))
    {
        is_random_animation_timer_running = false;
        return ALARM_STOP;
    }
    else
    {
        /* If the work queue is full we'll try again later. */
        return ALARM_RESCHEDULE_AFTER_MS(RANDOM_ANIMATION_PERIOD_MS);
    }
}

static bool adc_read_callback(repeating_timer_t *timer)
{
    (void)timer;
    work_item_t work = {
        .destination = WORK_MODULE_MAIN,
        .command = WORK_ITEM_READ_ADC_SENSORS,
    };
    work_queue_try_add(work);
    /* We never want to stop. If the work queue is full this just results in temporarily slower
     * brightness updates. */
    return REPEATING_TIMER_CONTINUE;
}

static bool osd_update_callback(repeating_timer_t *timer)
{
    (void)timer;
    work_item_t work = {
        .destination = WORK_MODULE_MAIN,
        .command = WORK_ITEM_UPDATE_OSD,
    };
    work_queue_try_add(work);
    /* We never want to stop. If the work queue is full this just results in a temporarily lower
     * framerate. */
    return REPEATING_TIMER_CONTINUE;
}
