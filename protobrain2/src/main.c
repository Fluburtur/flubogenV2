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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <pico/assert.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include "adc_sensors.h"
#include "animation/anim.h"
#include "animation/animation_manager.h"
#include "ff.h"
#include "leds/led_brightness.h"
#include "leds/leds.h"
#include "main_work.h"
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

#define REPEATING_TIMER_CONTINUE true

/***********************
 * Variables
 ***********************/

static repeating_timer_t adc_read_timer;
static repeating_timer_t osd_update_timer;

/** The logo colour chosen by the user, before any brightness adjustment. */
static ws2812b_led_value_t logo_base_colour;
/** The logo colour we'll use, after brightness adjustment. */
static ws2812b_led_value_t logo_colour;

/***********************
 * Function prototypes
 ***********************/

static void do_work(work_item_t work);
static bool adc_read_callback(repeating_timer_t *timer);
static bool osd_update_callback(repeating_timer_t *timer);
static ws2812b_led_value_t load_logo_colour(void);

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

    logo_base_colour = load_logo_colour();
    logo_colour = led_brightness_get_logo_colour(logo_base_colour);
    leds_set_channel_to_colour(LED_CHANNEL_CHEEK, logo_colour, false);
    leds_set_channel_to_colour(LED_CHANNEL_BODY0, logo_colour, false);
    leds_set_channel_to_colour(LED_CHANNEL_BODY1, logo_colour, false);

    osd_init();

    /* Starts the boot animation. */
    animation_manager_init();

    remote_init();

    /* Start the timers last, so we don't accumulate lots of work during any slow parts of init. */
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

        case WORK_MODULE_ANIMATION:
            animation_manager_handle_work(work);
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
    case MAIN_WORK_CMD_READ_ADC_SENSORS:
    {
        bool averages_updated = adc_sensors_read();
        if (averages_updated)
        {
            /* "In the locked state the LED brightness does not change" */
            if (!animation_manager_is_locked())
            {
                led_brightness_update(adc_sensors_get_averages().brightness);

                /* Logo auto-brightness adjustment. */
                logo_colour = led_brightness_get_logo_colour(logo_base_colour);
            }

            /* Even when we're locked we still write to the logos, re-applying the same unchanged
             * value. This is for reliability; if a glitch/noise causes the LEDs to receive a bad
             * value, it will soon be replaced by a (hopefully) good value. */
            leds_set_channel_to_colour(LED_CHANNEL_CHEEK, logo_colour, false);
            leds_set_channel_to_colour(LED_CHANNEL_BODY0, logo_colour, false);
            leds_set_channel_to_colour(LED_CHANNEL_BODY1, logo_colour, false);
        }
    }
    break;

    case MAIN_WORK_CMD_UPDATE_OSD:
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
        /* Unrecognised work, something has gone wrong. */
        hard_assert(false);
        break;
    }
}

static bool adc_read_callback(repeating_timer_t *timer)
{
    (void)timer;
    work_item_t work = {
        .destination = WORK_MODULE_MAIN,
        .command = MAIN_WORK_CMD_READ_ADC_SENSORS,
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
        .command = MAIN_WORK_CMD_UPDATE_OSD,
    };
    work_queue_try_add(work);
    /* We never want to stop. If the work queue is full this just results in a temporarily lower
     * framerate. */
    return REPEATING_TIMER_CONTINUE;
}

/**
 * Read the cheek and body logo colour from the SD card (or use a default colour if the SD card
 * is disabled at compile time).
 */
static ws2812b_led_value_t load_logo_colour(void)
{
    /* Default to red for "error" */
    static const ws2812b_led_value_t default_red_colour = (ws2812b_led_value_t){.r = 255, .g = 0, .b = 0};

#ifdef USE_SD_CARD
    FIL handle;
    FRESULT result = f_open(&handle, "logo.txt", FA_READ);
    if (result != FR_OK)
    {
        return default_red_colour;
    }

    /* Read up to 8 bytes. This supports hex values formatted like `rr gg bb` or `rrggbb` */
    #define MAX_BYTES_TO_READ 8
    char buf[MAX_BYTES_TO_READ + 1] = {0};
    UINT n_bytes_got;
    result = f_read(&handle, buf, MAX_BYTES_TO_READ, &n_bytes_got);
    if (result != FR_OK)
    {
        return default_red_colour;
    }

    /* By limiting the format to 2 characters of unsigned hex, we don't need to check the value
     * ranges manually. sscanf() will automatically clamp it to 0-255. */
    const char* format = "%2" SCNx8 "%2" SCNx8 "%2" SCNx8;
    uint8_t r, g, b;
    int parse_result = sscanf(buf, format, &r, &g, &b);
    if ((parse_result == EOF) || (parse_result != 3))
    {
        return default_red_colour;
    }
    return (ws2812b_led_value_t){.r = r, .g = g, .b = b};
#else
    return default_red_colour;
#endif
}
