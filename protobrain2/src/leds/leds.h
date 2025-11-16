#ifndef _LEDS_H_
#define _LEDS_H_

/**
 * High level driver for the WS2812B LED displays.
 */

#include "ws2812b.h"

#define NUM_LED_CHANNELS 4

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
void leds_debug_set_channel_to_colour(led_channel_t channel, ws2812b_led_value_t colour, bool flush);

#endif /* #ifndef _LEDS_H_ */
