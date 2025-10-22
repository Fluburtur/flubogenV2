#include "main.h"
#include "leds.h"
#include <math.h>

uint8_t gammaCorrection[256];

brightness_level_t brightnessLevels[5] = {
	{1300, MIN_BRIGHTNESS},
	{2300, 20},
	{3100, INITIAL_BRIGHTNESS},
	{3500, 100},
	{4095, MAX_BRIGHTNESS},
};

const uint8_t brightnessLevelsCount = sizeof(brightnessLevels) / sizeof(brightnessLevels[0]);

const uint16_t lightValues[] = {1300, 2300, 3100, 3500, 4095};

// WS2812B
// 0: T0H T0L	250n-550n(400n) 700n-1000n(850n)
// 1: T1H T1L	650n-950n(800n) 300n-600n(450n)
// RET: Treset >50u
// 120ns steps, 1.2us period:
// 0: ###-----	4:480n/720n
// 1: #####---	7:840n/360n
// Period: 9: 1200ns
// Cleanflight driver does 350n/800n for 0, 700n/650n for 1

void makePWMBuffer(uint32_t * PWMBuffer, rgbColor24bpp_t * colorBufferA, uint32_t ledCountA, rgbColor24bpp_t * colorBufferB, uint32_t ledCountB) {
	// Convert color buffers to PWM buffer
	uint32_t i = 0;

	// Channel A
	for (uint32_t led = 0; led < ledCountA; led++) {
		uint32_t colorRaw = colorBufferA[led].rawlong;
		// This gives BBGGRR, WS2812 wants GGRRBB
		colorRaw = (colorRaw << 8) | ((colorRaw >> 16) & 0xFF);
		for (int bit = 0; bit < 24; bit++) {
			//PWMBuffer[i] = ((colorRaw << bit) & 0x800000ul) ? 7 : 4;	// 120ns steps
			PWMBuffer[i] = ((colorRaw << bit) & 0x800000ul) ? 18 : 9;	// 40ns steps
			i++;
		}
	}

	// Channel B
	i = 0;
	for (uint32_t led = 0; led < ledCountB; led++) {
		uint32_t colorRaw = colorBufferB[led].rawlong;
		// This gives BBGGRR, WS2812 wants GGRRBB
		colorRaw = (colorRaw << 8) | ((colorRaw >> 16) & 0xFF);
		for (int bit = 0; bit < 24; bit++) {
			//PWMBuffer[i] |= ((colorRaw << bit) & 0x800000ul) ? (7 << 16) : (4 << 16);	// 120ns steps
			PWMBuffer[i] |= ((colorRaw << bit) & 0x800000ul) ? (18 << 16) : (9 << 16);	// 40ns steps
			i++;
		}
	}
}

void refreshLEDs() {
	makePWMBuffer(PWMBufferFaceCheek, colorBufferFace, FACE_LED_STRIP_LENGTH, colorBufferCheek, CHEEK_LED_STRIP_LENGTH);
	makePWMBuffer(PWMBufferBody0Body1, colorBufferBody0, BODY0_LED_STRIP_LENGTH, colorBufferBody1, BODY1_LED_STRIP_LENGTH);
	// TODO: Make sure last values are 0 to force outputs low

	// Start appropriate DMA channels
	dma_channel_set_read_addr(PWMDMAChFaceCheek, PWMBufferFaceCheek, true);
	dma_channel_set_read_addr(PWMDMAChBody0Body1, PWMBufferBody0Body1, true);
}

void checkBrightness() {
	//LOG_E(SYSTEM, "RSSI ADC: %d, brightness: %d", adcGetChannel(ADC_RSSI), brightness);
	changeAutoBrightness();

	if (resetBrightness) {
		initialiseAutoBrightness();
		brightnessAdjustment = 0;
		changeBrightness = true;
		resetBrightness = false;
	}

	if (changeBrightness) {
		//LOG_E(SYSTEM, "Updating brightness...");

		if (brightness + brightnessAdjustment > MAX_BRIGHTNESS)
			updateGammaCorrection(MAX_BRIGHTNESS);
		else if (brightness + brightnessAdjustment < MIN_BRIGHTNESS)
			updateGammaCorrection(MIN_BRIGHTNESS);
		else
			updateGammaCorrection(brightness + brightnessAdjustment);

		//LOG_E(SYSTEM, "Brightness changed to %d", brightness);
		changeBrightness = false;
	}
}

void updateGammaCorrection(double brightness) {
	// TODO: Make this fixed-point
	double gamma = MAX_GAMMA - (((double)255.0 - brightness)/(double)255.0) * (MAX_GAMMA - MIN_GAMMA);
	double a = brightness / pow(255, gamma);

	for (uint8_t i = 0;; i++) {
		gammaCorrection[i] = round(a * pow(i, gamma));

		if (i == 255)
			break;
	}
}

uint8_t lightInterval(uint16_t adcValue) {
	// Find in which interval (lightValues) the measured brightness is
	for (uint8_t i = 0; i < brightnessLevelsCount; i++) {
		if (adcValue < brightnessLevels[i].adc_max)
			return i;
	}

	return brightnessLevelsCount - 1;
}

void initialiseAutoBrightness() {
	lastLightInterval = 0;	// TODO 	//lightInterval(adcGetChannel(ADC_RSSI));
	brightness = brightnessLevels[lastLightInterval].brightness;
	printDebug("Brightness initialised to %u\n", brightness);
}

void changeAutoBrightness() {
	uint8_t newLightInterval = 0;	// TODO 	//lightInterval(adcGetChannel(ADC_RSSI));
	uint8_t newBrightness = brightnessLevels[newLightInterval].brightness;

	if (newBrightness != brightness && newLightInterval != lastLightInterval) {
		brightness = newBrightness;
		lastLightInterval = newLightInterval;
		changeBrightness = true;
	}
}