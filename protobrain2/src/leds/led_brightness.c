#include <math.h>

#include "led_brightness.h"

#include "adc_sensors.h"
#include "misc.h"

#define MIN_BRIGHTNESS 10
#define INITIAL_BRIGHTNESS 50
/** The hardware can be driven up to 255, but we limit to 150 due to power supply limitations. */
#define MAX_BRIGHTNESS 150

#define MAX_GAMMA 1.9
#define MIN_GAMMA 1.2

/* Maps an ADC sensor value to an automatic brightness level. */
typedef struct
{
    /* Input ADC sensor value. */
    uint16_t adc_max;
    /* Output brightness level. */
    uint8_t brightness;
} brightness_level_t;

/* Maps an ADC sensor value to an automatic brightness level.
 * Must be ordered from low to high sensor value. */
static const brightness_level_t auto_brightness_levels[] = {
    {1300, MIN_BRIGHTNESS},
    {2300, 20},
    {3100, INITIAL_BRIGHTNESS},
    {3500, 100},
    {4095, MAX_BRIGHTNESS},
};

/* The .fur file has the animation's RGB values in the range 0-255.
 * Map these to the range 0 to `auto_brightness_limit` */
static uint8_t brightnessMap[256] = {};

/* The automatically chosen brightness limit. */
static uint8_t auto_brightness_limit;

static uint8_t sensor_value_to_auto_brightness(uint16_t adc_value);
static void update_brightness_map(double brightness_limit);

void led_brightness_init(uint16_t adc_brightness)
{
    /* Set to an invalid value to force an update. */
    auto_brightness_limit = 0;
    led_brightness_update_auto(adc_brightness);

    printDebug("Brightness initialised to %u\n", auto_brightness_limit);
}

void led_brightness_update_auto(uint16_t adc_brightness)
{
    uint8_t old_auto_brightness_limit = auto_brightness_limit;

    auto_brightness_limit = sensor_value_to_auto_brightness(adc_brightness);
    if (auto_brightness_limit != old_auto_brightness_limit)
    {
        update_brightness_map(auto_brightness_limit);
    }
}

const uint8_t *led_brightness_get_map(void)
{
    return brightnessMap;
}

/**
 * Convert an ADC sensor value to an automatic brightness.
 */
static uint8_t sensor_value_to_auto_brightness(uint16_t adc_value)
{
    uint8_t last_table_idx = ARRAY_NELEMS(auto_brightness_levels) - 1;
    /* The mapping is step-wise.
     * The brightness is x until the first sensor value, y until the second sensor value... */
    for (uint8_t i = 0; i <= last_table_idx; i++)
    {
        if (adc_value < auto_brightness_levels[i].adc_max)
        {
            return auto_brightness_levels[i].brightness;
        }
    }

    return auto_brightness_levels[last_table_idx].brightness;
}

/**
 * Update the brightness map
 *
 * @param[in] brightness_limit The chosen brightness limit.
 */
static void update_brightness_map(double brightness_limit)
{
    // TODO: Make this fixed-point
    double gamma = MAX_GAMMA - ((255.0 - brightness_limit) / 255.0) * (MAX_GAMMA - MIN_GAMMA);
    double a = brightness_limit / pow(255.0, gamma);

    for (uint16_t i = 0; i < 256; i++)
    {
        brightnessMap[i] = round(a * pow(i, gamma));
    }
}
