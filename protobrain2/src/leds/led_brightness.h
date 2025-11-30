/*
 * Automatic and manual LED brightness adjustment for the face only.
 *
 * LED brightness is updated continuously, based on an ambient light sensor.
 * A user offset can be applied to the automatic value.
 *
 * The brightness adjustment serves 3 purposes:
 *   - Limit the max brightness so we don't draw too much power.
 *   - Automatically adjust the brightness so we don't blind people in dark rooms.
 *   - A linear input to a non-linear output, for gamma correction.
 *
 * Note: whatever the user offset is set to, it will not cause the max brightness to be exceeded.
 * I.e. the power limit is always maintained.
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
void led_brightness_update_auto(uint16_t adc_brightness);

/**
 * Clear the user offset and update the brightness.
 */
void led_brightness_clear_user_offset(void);

/**
 * Increase the user offset by one step, and update the brightness.
 */
void led_brightness_increase_user_offset(void);

/**
 * Decrease the user offset by one step, and update the brightness.
 */
void led_brightness_decrease_user_offset(void);

/**
 * Get the LED brightness map.
 *
 * The map contains 255 values.
 * The index is an input R/G/B value in the range 0-255.
 * The output is an R/G/B value in the range 0 to the safe value.
 */
const uint8_t *led_brightness_get_map(void);

#endif /* _LED_BRIGHTNESS_H_ */
