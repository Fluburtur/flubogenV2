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

#include "leds/leds.h"

int main(void) {
    hard_assert(stdio_init_all());

    leds_init();
    sleep_ms(50);

    // The face LEDs should be capped to 100 out of 255 to limit power draw.
    //
    // Also note that the perceived brightness of each channel is not equal:
    //   Red 405   Green 690   Blue 190
    // So if we normalise around the capability of the blue channel, for equal
    // brightness the channels should be scaled like:
    //   Red 0.47  Green 0.28  Blue 1.0
    // I won't do the brightness equalisation now, though. Just the power limit.

    ws2812b_led_value_t red = {.r = 70, .g = 0, .b = 0};
    ws2812b_led_value_t green = {.r = 0, .g = 70, .b = 0};
    ws2812b_led_value_t blue = {.r = 0, .g = 0, .b = 70};
    ws2812b_led_value_t white = {.r = 70, .g = 70, .b = 70};

    // Loop: r g b white, but each channel is offset through the sequence.
    // The power limit is also applied to the non-face channels, because I'm lazy (and it's safer
    // if I've made a mistake).
    while (1)
    {
        leds_debug_set_channel_to_colour(LED_CHANNEL_FACE, red, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_CHEEK, green, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY0, blue, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY1, white, false);
        sleep_ms(1000);

        leds_debug_set_channel_to_colour(LED_CHANNEL_FACE, green, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_CHEEK, blue, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY0, white, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY1, red, false);
        sleep_ms(1000);

        leds_debug_set_channel_to_colour(LED_CHANNEL_FACE, blue, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_CHEEK, white, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY0, red, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY1, green, false);
        sleep_ms(1000);

        leds_debug_set_channel_to_colour(LED_CHANNEL_FACE, white, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_CHEEK, red, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY0, green, false);
        leds_debug_set_channel_to_colour(LED_CHANNEL_BODY1, blue, false);
        sleep_ms(1000);
    }
}
