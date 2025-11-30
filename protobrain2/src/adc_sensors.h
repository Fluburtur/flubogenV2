/*
 * Sensors that are read via the ADC: battery voltage, battery current, brightness.
 */

#ifndef _ADC_SENSORS_H_
#define _ADC_SENSORS_H_

#include <stdbool.h>
#include <stdint.h>

/** Sensor values. */
typedef struct
{
    /** TODO: units and range */
    uint16_t battery_v;
    /** TODO: units and range */
    uint16_t battery_i;
    /** TODO: units and range */
    uint16_t brightness;
} adc_sensors_values_t;

/**
 * Initialise the ADC sensors module.
 */
void adc_sensors_init(void);

/**
 * Read all of the sensors.
 *
 * @returns `true` if the average values have been updated.
 */
bool adc_sensors_read(void);

/**
 * Get the current set of average values.
 */
adc_sensors_values_t adc_sensors_get_averages(void);

#endif /* _ADC_SENSORS_H_ */
