#include <math.h>

#include <pico/assert.h>

#include "led_brightness.h"

#include "adc_sensors.h"
#include "misc.h"

/** The hardware can be driven up to 255, but we limit to 150 due to power supply limitations. */
#define MAX_BRIGHTNESS_FACE 150

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
static const brightness_level_t auto_brightness_levels_face[] = {
    {1300, 10},
    {2300, 20},
    {3100, 50},
    {3500, 100},
    {4095, MAX_BRIGHTNESS_FACE},
};

/* The .fur file has the animation's RGB values in the range 0-255.
 * Map these to the range 0 to `brightness_limit_face` */
static uint8_t brightness_map_face[256] = {};

/* The automatically chosen brightness limit. */
static uint8_t brightness_limit_face;

static uint8_t sensor_value_to_auto_brightness_face(uint16_t adc_value);
static void update_brightness_map_face(double brightness_limit);

void led_brightness_init(uint16_t adc_brightness)
{
    /* Set to an invalid value to force an update. */
    brightness_limit_face = 0;
    led_brightness_update(adc_brightness);

    printDebug("Face brightness initialised to %u\n", brightness_limit_face);
}

void led_brightness_update(uint16_t adc_brightness)
{
    uint8_t old_brightness_limit_face = brightness_limit_face;

    brightness_limit_face = sensor_value_to_auto_brightness_face(adc_brightness);
    if (brightness_limit_face != old_brightness_limit_face)
    {
        update_brightness_map_face(brightness_limit_face);
    }
}

const uint8_t *led_brightness_get_face_map(void)
{
    return brightness_map_face;
}

/**
 * Convert an ADC sensor value to an automatic brightness for the face.
 */
static uint8_t sensor_value_to_auto_brightness_face(uint16_t adc_value)
{
    uint8_t last_table_idx = ARRAY_NELEMS(auto_brightness_levels_face) - 1;
    /* The mapping is step-wise.
     * The brightness is x until the first sensor value, y until the second sensor value... */
    for (uint8_t i = 0; i <= last_table_idx; i++)
    {
        if (adc_value < auto_brightness_levels_face[i].adc_max)
        {
            return auto_brightness_levels_face[i].brightness;
        }
    }

    return auto_brightness_levels_face[last_table_idx].brightness;
}

/**
 * Update the face brightness map.
 *
 * @param[in] brightness_limit The chosen brightness limit.
 */
static void update_brightness_map_face(double brightness_limit)
{
    hard_assert(brightness_limit <= (double)MAX_BRIGHTNESS_FACE);

    // TODO: Make this fixed-point
    double gamma = MAX_GAMMA - ((255.0 - brightness_limit) / 255.0) * (MAX_GAMMA - MIN_GAMMA);
    double a = brightness_limit / pow(255.0, gamma);

    for (uint16_t i = 0; i < 256; i++)
    {
        brightness_map_face[i] = round(a * pow(i, gamma));
    }
}
