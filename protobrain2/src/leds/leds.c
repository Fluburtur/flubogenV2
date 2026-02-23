/**
 * High level driver for the WS2812B LED displays.
 *
 * This module owns some data buffers for each display channel. A buffer stores the colour values
 * for each LED in its display.
 */

#include <stdint.h>

#include "leds.h"

#define GPIO_PIN_FACE 12
#define GPIO_PIN_CHEEK 13
#define GPIO_PIN_BODY0 14
#define GPIO_PIN_BODY1 15

/* Stores the channel data for the low-level driver. */
static ws2812b_channel_t channels[NUM_LED_CHANNELS];

static ws2812b_led_value_t face_buffer[NUM_LEDS_FACE];
static ws2812b_led_value_t cheek_buffer[NUM_LEDS_CHEEK];
static ws2812b_led_value_t body0_buffer[NUM_LEDS_BODY0];
static ws2812b_led_value_t body1_buffer[NUM_LEDS_BODY1];

/* A lookup from `led_channel_t` to buffer. */
static ws2812b_led_value_t *const channel_buffers[NUM_LED_CHANNELS] =
{
    face_buffer, cheek_buffer, body0_buffer, body1_buffer,
};

void leds_init(void)
{
    /* Channel initialisation data. Must match the order within `led_channel_t` */
    static const ws2812b_channel_init_t channel_init[NUM_LED_CHANNELS] =
    {
        {.gpio_pin = GPIO_PIN_FACE, .num_leds = NUM_LEDS_FACE},
        {.gpio_pin = GPIO_PIN_CHEEK, .num_leds = NUM_LEDS_CHEEK},
        {.gpio_pin = GPIO_PIN_BODY0, .num_leds = NUM_LEDS_BODY0},
        {.gpio_pin = GPIO_PIN_BODY1, .num_leds = NUM_LEDS_BODY1},
    };

    ws2812b_init(channel_init, channels, NUM_LED_CHANNELS);
}

void leds_set_channel_to_colour(led_channel_t channel, ws2812b_led_value_t colour, bool flush)
{
    ws2812b_channel_t channel_info = channels[channel];
    ws2812b_led_value_t *buffer = leds_get_buffer_for_channel(channel);

    for (uint i = 0; i < channel_info.num_leds; i++)
    {
        buffer[i] = colour;
    }

    ws2812b_send_buffer(&channel_info, buffer);
    if (flush)
    {
        ws2812b_wait_for_idle(&channel_info);
    }
}

void leds_write_buffer_to_channel(led_channel_t channel)
{
    ws2812b_channel_t channel_info = channels[channel];
    ws2812b_led_value_t *buffer = leds_get_buffer_for_channel(channel);
    ws2812b_send_buffer(&channel_info, buffer);
}

ws2812b_led_value_t *leds_get_buffer_for_channel(led_channel_t channel)
{
    return channel_buffers[channel];
}
