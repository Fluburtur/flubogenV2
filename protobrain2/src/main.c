// Protobrain V2 for RP2040

// OSD chip might not work if powered by long/shitty USB cable, requires 5V
// On startup all GPIOs are in hi-z with a pull-down

// TODO: Current sense, auto brightness scale down if above max current
// TOOD: Interactive console to set values, save/load to txt file

// NOTE: The old MAX7456 driver assumes that the chip was loaded with a modified charset to match the standard ASCII table,
// this won't work with a brand new MAX7456.

// ADC values for automatic brightness adjustment (GL5528 photoresistor + 10k pulldown)

#include <stdint.h>

#include <pico/assert.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include "adc_sensors.h"
#include "anim.h"
#include "leds/led_brightness.h"
#include "leds/leds.h"
#include "work_queue.h"

/** Read the ADC sensors at 100Hz, producing average values at 10Hz. */
#define ADC_READ_PERIOD_MS 10

static repeating_timer_t face_animation_timer;
static repeating_timer_t adc_read_timer;

static bool face_animation_callback(repeating_timer_t *timer);
static bool adc_read_callback(repeating_timer_t *timer);

int main(void)
{
    hard_assert(stdio_init_all());

    work_queue_init();

    adc_sensors_init();
    led_brightness_init(adc_sensors_get_averages().brightness);

    leds_init();
    sleep_ms(1);

    hard_assert(animationInit());
    animationSetLocked(true);
    uint16_t animation_period_ms = startAnimation(BOOT_ANIMATION);
    hard_assert(animation_period_ms != 0);
    hard_assert(
        add_repeating_timer_ms(
            animation_period_ms, face_animation_callback, NULL, &face_animation_timer));
    hard_assert(
        add_repeating_timer_ms(
            ADC_READ_PERIOD_MS, adc_read_callback, NULL, &adc_read_timer));

    // Note that the perceived brightness of each channel is not equal:
    //   Red 405   Green 690   Blue 190
    // So if we normalise around the capability of the blue channel, for equal
    // brightness the channels should be scaled like:
    //   Red 0.47  Green 0.28  Blue 1.0
    // I won't do the brightness equalisation now, though.

    ws2812b_led_value_t red = {.r = 150, .g = 0, .b = 0};
    ws2812b_led_value_t green = {.r = 0, .g = 150, .b = 0};
    ws2812b_led_value_t blue = {.r = 0, .g = 0, .b = 150};
    ws2812b_led_value_t white = {.r = 150, .g = 150, .b = 150};
    ws2812b_led_value_t colour_cycle[4] = {red, green, blue, white};

    // * Play the face animation (20 FPS).
    // * Cycle the cheek, body0, and body1 panels through R G B White (1 fps), but each channel is
    //   offset through the sequence.
    // I've also limited the power of the non-face channels to 150, because it's safer if I've made
    // a mistake.
    uint8_t frame = 0;
    uint8_t colour_idx_cheek = 0;
    uint8_t colour_idx_body0 = 1;
    uint8_t colour_idx_body1 = 2;

    while (true)
    {
        work_item_t work = work_queue_remove_blocking();
        switch (work)
        {
        case WORK_ITEM_ANIMATE_FACE_FRAME:
        {
            frame++;
            updateAnimation();

            if (frame == 1)
            {
                leds_set_channel_to_colour(LED_CHANNEL_CHEEK, colour_cycle[colour_idx_cheek], false);
                leds_set_channel_to_colour(LED_CHANNEL_BODY0, colour_cycle[colour_idx_body0], false);
                leds_set_channel_to_colour(LED_CHANNEL_BODY1, colour_cycle[colour_idx_body1], false);
                colour_idx_cheek = (colour_idx_cheek + 1) % 4;
                colour_idx_body0 = (colour_idx_body0 + 1) % 4;
                colour_idx_body1 = (colour_idx_body1 + 1) % 4;
            }

            sleep_ms(50);

            if (frame == 20)
            {
                frame = 0;
            }
        }
        break;

        case WORK_ITEM_READ_ADC_SENSORS:
        {
            bool averages_updated = adc_sensors_read();
            if (averages_updated)
            {
                led_brightness_update_auto(adc_sensors_get_averages().brightness);
            }
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
}

static bool face_animation_callback(repeating_timer_t *timer)
{
    (void)timer;
    work_queue_add(WORK_ITEM_ANIMATE_FACE_FRAME);
    /* Assume we want to draw more frames. */
    return true;
}

static bool adc_read_callback(repeating_timer_t *timer)
{
    (void)timer;
    work_queue_add(WORK_ITEM_READ_ADC_SENSORS);
    /* We never want to stop. */
    return true;
}
