/*
 * The battery voltage sensor is a voltage divider.
 * The battery current sensor is an INA138 current shunt monitor.
 * The brightness sensor is a voltage divider with a generic photoresistor.
 */

#include "adc_sensors.h"

#include <hardware/adc.h>
#include <hardware/gpio.h>

#define GPIO_PIN_BATTERY_VOLTAGE 26
#define GPIO_PIN_BATTERY_CURRENT 27
#define GPIO_PIN_BRIGHTNESS 28

#define PULL_UP_NO false
#define PULL_DOWN_YES true

#define ADC_INPUT_BATTERY_VOLTAGE 0
#define ADC_INPUT_BATTERY_CURRENT 1
#define ADC_INPUT_BRIGHTNESS 2

#define AVERAGE_COUNT 10

/* Accumulates values for averaging. */
static uint32_t accum_battery_v;
static uint32_t accum_battery_i;
static uint32_t accum_brightness;
/* How many times we have accumulated. */
static uint8_t accum_counter;
/* The averaged values. */
adc_sensors_values_t averages;

void adc_sensors_init(void)
{
    adc_gpio_init(GPIO_PIN_BATTERY_VOLTAGE);
    adc_gpio_init(GPIO_PIN_BATTERY_CURRENT);

    /* The old main.c set up the light sensor pin for GPIO, including a pull-down.
     * Notably it didn't call adc_gpio_init().
     * This might be a bit strange, but it works, and I don't want to mess with it. */
    gpio_init(GPIO_PIN_BRIGHTNESS);
    gpio_set_pulls(GPIO_PIN_BRIGHTNESS, PULL_UP_NO, PULL_DOWN_YES);

    adc_init();

    accum_battery_v = 0;
    accum_battery_i = 0;
    accum_brightness = 0;
    accum_counter = 0;

    /* Take a single set of readings. We'll use them as our "average" values until we have enough
     * time to get an actual average. */
    adc_sensors_read();
    averages.battery_v = accum_battery_v;
    averages.battery_i = accum_battery_i;
    averages.brightness = accum_brightness;
}

bool adc_sensors_read(void)
{
    // 3.3VRef 12-bit, 805.66uV/step
    adc_select_input(ADC_INPUT_BATTERY_VOLTAGE);
    accum_battery_v += adc_read();

    adc_select_input(ADC_INPUT_BATTERY_CURRENT);
    accum_battery_i += adc_read();

    adc_select_input(ADC_INPUT_BRIGHTNESS);
    accum_brightness += adc_read();

    accum_counter++;
    if (accum_counter >= AVERAGE_COUNT)
    {
        averages.battery_v = accum_battery_v / AVERAGE_COUNT;
        averages.battery_i = accum_battery_i / AVERAGE_COUNT;
        averages.brightness = accum_brightness / AVERAGE_COUNT;

        accum_battery_v = 0;
        accum_battery_i = 0;
        accum_brightness = 0;
        accum_counter = 0;

        return true;
    }
    return false;
}

adc_sensors_values_t adc_sensors_get_averages(void)
{
    return averages;
}
