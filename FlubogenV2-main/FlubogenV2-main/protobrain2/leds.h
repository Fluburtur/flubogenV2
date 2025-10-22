#ifndef _LEDS_H_
#define _LEDS_H_

#define MAX_GAMMA (double) 1.9
#define MIN_GAMMA (double) 1.2

#define MAX_LEDS_PER_OUTPUT 512

// Max framerate is 1 / (312 LEDs * 24 bits per LED * 1.2us per bit) = 111 Hz
#define FACE_LED_STRIP_LENGTH 312
#define CHEEK_LED_STRIP_LENGTH 9
#define BODY0_LED_STRIP_LENGTH 9    // What is this ?
#define BODY1_LED_STRIP_LENGTH 41   // Cuissettes

#define LED_DATA_SIZE(x) ((x + 1) * 24)   // In bits, with last bit used to set outputs low

// These must go by pair because PWM channels do
typedef enum {
    LEDS_FACE_CHEEK = 0,
    LEDS_BODY0_BODY1
} led_output_t;

typedef struct {
	uint16_t adc_max;	// In
	uint8_t brightness;	// Out
} brightness_level_t;

extern uint8_t gammaCorrection[256];

void checkBrightness(void);
void updateGammaCorrection(double brightness);
uint8_t lightInterval(uint16_t adcValue);
void initialiseAutoBrightness(void);
void changeAutoBrightness(void);
void refreshLEDs();

#endif /* _LEDS_H_ */
