#include "led_brightness.h"

#include "adc_sensors.h"
#include "misc.h"

#define MIN_BRIGHTNESS 10
#define INITIAL_BRIGHTNESS 50
/** The hardware can be driven up to 255, but we limit to 150 due to power supply limitations. */
#define MAX_BRIGHTNESS 150

typedef struct
{
    uint16_t adc_max;   // In
    uint8_t brightness; // Out
} brightness_level_t;

static const brightness_level_t brightnessLevels[] = {
    {1300, MIN_BRIGHTNESS},
    {2300, 20},
    {3100, INITIAL_BRIGHTNESS},
    {3500, 100},
    {4095, MAX_BRIGHTNESS},
};

static const uint8_t brightnessLevelsCount = sizeof(brightnessLevels) / sizeof(brightnessLevels[0]);

/** The actual brightness limit to use. */
static volatile uint8_t brightness = INITIAL_BRIGHTNESS;
/** Desire to change brightness. Set by remote brightness up/down command, resetBrightness, or auto brightness control from light sensor. */
static volatile bool changeBrightness = false;
/** Desire to reset brightness to the current automatic level. Set by remote reset command. */
static volatile bool resetBrightness = false;
/** Internal var, part of the auto brightness. */
static uint8_t lastLightInterval;
/** This is a manual offset to be applied after the auto brightness value. */
static int16_t brightnessAdjustment = 0;

static void changeAutoBrightness(uint16_t adc_brightness);
static uint8_t lightInterval(uint16_t adcValue);
static void updateGammaCorrection(double brightness);

void initialiseAutoBrightness()
{
    lastLightInterval = 0; // TODO 	//lightInterval(adcGetChannel(ADC_RSSI));
    brightness = brightnessLevels[lastLightInterval].brightness;
    printDebug("Brightness initialised to %u\n", brightness);
}

void checkBrightness(uint16_t adc_brightness)
{
    // LOG_E(SYSTEM, "RSSI ADC: %d, brightness: %d", adcGetChannel(ADC_RSSI), brightness);
    changeAutoBrightness(adc_brightness);

    if (resetBrightness)
    {
        initialiseAutoBrightness();
        brightnessAdjustment = 0;
        changeBrightness = true;
        resetBrightness = false;
    }

    if (changeBrightness)
    {
        // LOG_E(SYSTEM, "Updating brightness...");

        if (brightness + brightnessAdjustment > MAX_BRIGHTNESS)
            updateGammaCorrection(MAX_BRIGHTNESS);
        else if (brightness + brightnessAdjustment < MIN_BRIGHTNESS)
            updateGammaCorrection(MIN_BRIGHTNESS);
        else
            updateGammaCorrection(brightness + brightnessAdjustment);

        // LOG_E(SYSTEM, "Brightness changed to %d", brightness);
        changeBrightness = false;
    }
}

static void changeAutoBrightness(uint16_t adc_brightness)
{
    // uint8_t newLightInterval = 0;	// TODO 	//lightInterval(adcGetChannel(ADC_RSSI));
    uint8_t newLightInterval = lightInterval(adc_brightness);
    uint8_t newBrightness = brightnessLevels[newLightInterval].brightness;

    if (newBrightness != brightness && newLightInterval != lastLightInterval)
    {
        brightness = newBrightness;
        lastLightInterval = newLightInterval;
        changeBrightness = true;
    }
}

static uint8_t lightInterval(uint16_t adcValue)
{
    // Find in which interval (lightValues) the measured brightness is
    for (uint8_t i = 0; i < brightnessLevelsCount; i++)
    {
        if (adcValue < brightnessLevels[i].adc_max)
            return i;
    }

    return brightnessLevelsCount - 1;
}

static void updateGammaCorrection(double brightness)
{
    (void)brightness;
    // TODO: hook into anim.c
}
