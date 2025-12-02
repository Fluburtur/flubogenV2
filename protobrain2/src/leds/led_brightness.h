/*
 * Automatic LED brightness adjustment for the face and logos.
 *
 * LED brightness is updated continuously, based on an ambient light sensor.
 *
 * The brightness adjustment serves 3 purposes:
 *   - Limit the max brightness so we don't draw too much power.
 *   - Automatically adjust the brightness so we don't blind people in dark rooms.
 *   - A linear input to a non-linear output, for gamma correction.
 */

#ifndef _LED_BRIGHTNESS_H_
#define _LED_BRIGHTNESS_H_

#include <stdint.h>

/**
 * Initialise the LED brightness module.
 *
 * @param[in] adc_brightness The current ADC brightness sensor value.
 */
void led_brightness_init(uint16_t adc_brightness);

/**
 * Update the brightness given an ADC brightness sensor value.
 *
 * @param[in] adc_brightness The current ADC brightness sensor value.
 */
void led_brightness_update(uint16_t adc_brightness);

/**
 * Get the LED brightness map for the face.
 *
 * The map contains 255 values.
 * The index is an input R/G/B value in the range 0-255.
 * The output is an R/G/B value in the range 0 to the safe value.
 */
const uint8_t *led_brightness_get_face_map(void);

/**
 * Get the brightness limit for the logos.
 *
 * Assumes the logos are set to a single colour.
 */
uint8_t led_brightness_get_logo_value(void);

#endif /* _LED_BRIGHTNESS_H_ */
