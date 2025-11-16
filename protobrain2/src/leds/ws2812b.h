#ifndef _WS2812B_H_
#define _WS2812B_H_

/**
 * Low level driver for the WS2812B LED displays.
 *
 * The driver operates with the concept of channels. A channel represents the driver for a
 * particular LED display.
 *
 * The user of this module is responsible for setting up each channel: which GPIO pin does it use,
 * and how many LEDs arein the display.
 */

#include <stdint.h>

#include <hardware/pio.h>

/**
 * The colour value for one LED.
 */
typedef union
{
    /** As a native 32 bit word: 8 bits green, red, blue, unused, from MSB to LSB. */
    uint32_t grbx;

    /** Or access to the individual bytes. */
    struct
    {
        // Little endian order!

        /** Unused. */
        uint8_t _x;
        uint8_t b;
        uint8_t r;
        uint8_t g;
    };
} ws2812b_led_value_t;

/**
 * Used to request a channel during initialisation.
 */
typedef struct
{
    /** Which GPIO pin to use. */
    uint gpio_pin;
    /** How many LEDs are in this channel. */
    uint num_leds;
} ws2812b_channel_init_t;

/**
 * A channel created during initialisation.
 *
 * This data should be considered private to this module.
 */
typedef struct
{
    /** Which PIO instance to use. */
    PIO pio_instance;
    /** Which SM instance to use. */
    uint sm_instance;
    /** Which DMA channel to use. */
    uint dma_channel;
    /** How many LEDs are in this channel. */
    uint num_leds;
} ws2812b_channel_t;

/**
 * Initialise the WS2812B driver.
 *
 * Pass an array of `ws2812b_channel_init_t` to request one or more LED channels.
 * On success, \p channel_results is updated with details of the channels created.
 * Caller is responsible for allocating appropriate and identical length arrays.
 *
 * @param[in] channel_requests The desired LED channels.
 * @param[in] num_channels How many channels.
 * @param[out] channel_results The channels that were created.
 */
void ws2812b_init(
    const ws2812b_channel_init_t *channel_requests, ws2812b_channel_t *channel_results,
    uint num_channels);

/**
 * Wait until a channel is idle.
 *
 * "Idle" means the LED display has been written to, and is ready to accept another frame.
 *
 * @param[in] channel The channel to wait for.
 */
void ws2812b_wait_for_idle(const ws2812b_channel_t *channel);

/**
 * Send a frame of data to a channel.
 *
 * A frame of data comprises the \p values for each LED in the display, in the order of nearest to
 * furthest.
 * The caller is responsible for ensuring that there are enough values provided.
 *
 * @param[in] channel The channel to wait for.
 * @param[in] values A buffer of values, one for each LED in this display.
 */
void ws2812b_send_buffer(const ws2812b_channel_t *channel, const ws2812b_led_value_t *values);

#endif /* #ifndef _WS2812B_H_ */
