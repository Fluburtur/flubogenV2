#ifndef _LEDS_H_
#define _LEDS_H_

/**
 * High level driver for the WS2812B LED displays.
 */

#include "ws2812b.h"

#define NUM_LED_CHANNELS 4

#define NUM_LEDS_FACE 312
#define NUM_LEDS_CHEEK 9
#define NUM_LEDS_BODY0 9
#define NUM_LEDS_BODY1 41

/** A particular LED display. */
typedef enum
{
    LED_CHANNEL_FACE,
    LED_CHANNEL_CHEEK,
    LED_CHANNEL_BODY0,
    LED_CHANNEL_BODY1,
} led_channel_t;

/**
 * Initialise the LEDs module.
 */
void leds_init(void);

/**
 * Set all LEDs in a display to the given \p colour
 *
 * @param[in] channel Which display to operate on.
 * @param[in] colour
 * @param[in] flush If `true`, wait until all data has been written to the display.
 */
void leds_set_channel_to_colour(led_channel_t channel, ws2812b_led_value_t colour, bool flush);

/**
 * Send the contents of the channel's buffer to the display.
 *
 * @param[in] channel Which display to operate on.
 */
void leds_write_buffer_to_channel(led_channel_t channel);

/**
 * Get a channel's buffer.
 *
 * @param[in] channel Which display to operate on.
 */
ws2812b_led_value_t *leds_get_buffer_for_channel(led_channel_t channel);

#endif /* #ifndef _LEDS_H_ */
