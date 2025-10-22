#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/resets.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "misc.h"
#include "anim.h"
#include "leds.h"
#include "ff.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

#define ADC_AVG_COUNT 10

#define FILE_NAME "file.fur"
#define RANDOM_ANIMATION_PERIOD 10000  // in ms
#define INITIAL_BRIGHTNESS 50
#define MAX_BRIGHTNESS 150  // absolute maximum is 255
#define MIN_BRIGHTNESS 10
#define BRIGHTNESS_STEP (MAX_BRIGHTNESS*5)/100

extern bool debugOutput;
extern bool cardOK;
extern int PWMDMAChFaceCheek, PWMDMAChBody0Body1;
extern rgbColor24bpp_t colorBufferFace[FACE_LED_STRIP_LENGTH];
extern rgbColor24bpp_t colorBufferCheek[CHEEK_LED_STRIP_LENGTH];
extern rgbColor24bpp_t colorBufferBody0[BODY0_LED_STRIP_LENGTH];
extern rgbColor24bpp_t colorBufferBody1[BODY1_LED_STRIP_LENGTH];
extern uint32_t PWMBufferFaceCheek[LED_DATA_SIZE(MAX_LEDS_PER_OUTPUT)];
extern uint32_t PWMBufferBody0Body1[LED_DATA_SIZE(MAX_LEDS_PER_OUTPUT)];
extern FIL filAnimation;
extern FIL filConfig;
extern uint8_t animationCount;
extern uint16_t animationTimer;
extern volatile bool changeBrightness, resetBrightness;
extern volatile uint8_t currentAnimation;
extern uint8_t animationCount, lastAnimation, lastLightInterval;
extern volatile uint8_t brightness;
extern volatile bool animationLocked;
extern int16_t brightnessAdjustment;

#endif /* _MAIN_H_ */
