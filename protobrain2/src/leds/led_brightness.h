/*
 * Automatic and manual LED brightness adjustment for the face only.
 *
 * LED brightness is updated continuously, based on an ambient light sensor.
 * A manual offset can be applied to the automatic value.
 */

#ifndef _LED_BRIGHTNESS_H_
#define _LED_BRIGHTNESS_H_

#include <stdbool.h>
#include <stdint.h>

void initialiseAutoBrightness(void);
void checkBrightness(uint16_t adc_brightness);

#endif /* _LED_BRIGHTNESS_H_ */
