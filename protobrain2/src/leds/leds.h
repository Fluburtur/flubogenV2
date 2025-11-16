#ifndef _LEDS_H_
#define _LEDS_H_

#include "ws2812b.h"

#define NUM_LED_CHANNELS 4

typedef enum
{
    LED_CHANNEL_FACE,
    LED_CHANNEL_CHEEK,
    LED_CHANNEL_BODY0,
    LED_CHANNEL_BODY1,
} led_channel_t;

void leds_init(void);
void leds_debug_set_channel_to_colour(led_channel_t channel, ws2812b_led_value_t value, bool flush);

#endif /* #ifndef _LEDS_H_ */
