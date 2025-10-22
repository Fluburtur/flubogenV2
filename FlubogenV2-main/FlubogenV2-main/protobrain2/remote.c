#include "main.h"
#include "remote.h"

// Each received byte must be saved in a FIFO
// After timeout without any byte received, process entire FIFO

void serialRxCallback(uint16_t data, void *rxCallbackData) {
	UNUSED(rxCallbackData);

	if (serialState == STATE_CMD) {
		switch (rxData) {
			case SET_ANIMATION:
				if (data == 0)
					currentAnimation = DEFAULT_ANIMATION;
				else
					currentAnimation = BOOT_ANIMATION + data;
				animationLocked = false;
				break;

			case LOCK_ANIMATION:
				if (data == 0)
					currentAnimation = DEFAULT_ANIMATION;
				else
					currentAnimation = BOOT_ANIMATION + data;
				animationLocked = true;
				break;

			case SET_BRIGHTNESS:
			
			/*	switch (data) {
					case DECREASE_BRIGHTNESS:
						changeBrightness = true;
						if (brightnessAdjustment - BRIGHTNESS_STEP > -MAX_BRIGHTNESS)
							brightnessAdjustment -= BRIGHTNESS_STEP;
						else
							brightnessAdjustment = -MAX_BRIGHTNESS;
						break;

					case INCREASE_BRIGHTNESS:
						changeBrightness = true;
						if (brightnessAdjustment + BRIGHTNESS_STEP < MAX_BRIGHTNESS)
							brightnessAdjustment += BRIGHTNESS_STEP;
						else
							brightnessAdjustment = MAX_BRIGHTNESS;
						break;

					case RESET_BRIGHTNESS:
						resetBrightness = true;
						break;
				}
				break;*/
		}
		serialState = STATE_PARAM1;
	} else if (serialState == STATE_PARAM1) {
		// TODO
		serialState = STATE_CMD;
	}
}
