#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "anim.h"
#include "leds.h"
#include "ff.h"
#include "delays.h"

const char *ANIMATION_NAME[] = {
	"NO ANIMATION",
	"DEFAULT ANIMATION",
	"RANDOM ANIMATION 1",
	"RANDOM ANIMATION 2",
	"RANDOM ANIMATION 3",
	"BOOT ANIMATION"
};

int32_t animationOffset[256];

uint8_t frameBitOffset(const uint8_t bitWidth, const uint32_t pixelsPerFame, const uint32_t frame) {
	return (bitWidth * pixelsPerFame * (frame & 7)) & 7;
}

// This only gets a list of offsets to the start of each animation data in the FUR file
bool animationInit(void) {
	UINT br;
	uint32_t offset = 1; // Start after animation count byte
	uint16_t frameCount;

	FRESULT fileOpened = f_open(&filAnimation, FILE_NAME, FA_READ);
	printDebug("File opened: %d\n", fileOpened);

	if (fileOpened != FR_OK)
		return false;

    f_read(&filAnimation, &animationCount, sizeof(uint8_t), &br);
	printDebug("Number of animations: %d\n", animationCount);

	for (uint8_t i = 0; i < animationCount; i++) {
		animationOffset[i] = offset;
		f_lseek(&filAnimation, offset);
		printDebug("Animation %d has offset %ld\n", i, animationOffset[i]);

		uint8_t fps;
    	f_read(&filAnimation, &fps, sizeof(uint8_t), &br);
		printDebug("\t%d FPS\n", fps & 0x7F);
		offset++;

        // MSB of FPS indicates indexed colors
		if (fps & 0x80) {
			printDebug("\tIndexed colors\n");
			uint16_t colorCount = 0;

			// Number of colors is saved as index of the last color
    		f_read(&filAnimation, &colorCount, sizeof(uint8_t), &br);
			offset++;
			colorCount++;
			printDebug("\t%d colors\n", colorCount);

            // Skip over color table
			f_lseek(&filAnimation, offset + 3 * colorCount);
			offset += (2 + 3 * colorCount);

			// Number of frames in animation
    		f_read(&filAnimation, &frameCount, sizeof(uint16_t), &br);
			byteSwap((uint8_t*)&frameCount);
			printDebug("\t%d frames\n", frameCount);

			offset += byteLength(frameCount * FACE_LED_STRIP_LENGTH, colorModeFromCount(colorCount));
		} else {
			printDebug("\tDirect colors\n");

    		f_read(&filAnimation, &frameCount, sizeof(uint16_t), &br);
			byteSwap((uint8_t*)&frameCount);
			printDebug("\t%d frames\n", frameCount);

			offset += (2 + 3 * frameCount * FACE_LED_STRIP_LENGTH);
		}
	}

	return true;
}

uint16_t animationDeltaMs;
color_mode_t animationColorMode;
uint32_t animationFrameStart;
uint32_t animationFrameOffset;
uint32_t animationFrameSize;		// How many bytes does an animation frame take
uint16_t animationFrameCount;
uint32_t animationFrame;
uint8_t animationCurrentNumber = 0;
rgbColor24bpp_t colorTable[256];

void startAnimation(uint8_t animationNumber) {
	uint16_t colorCount;	// Can reach 256
	UINT br;

	if (animationNumber == NO_ANIMATION) {
		// Clear
		memset(colorBufferFace, 0, sizeof(rgbColor24bpp_t) * FACE_LED_STRIP_LENGTH);
		refreshLEDs(LEDS_FACE_CHEEK);
		return;
	}

	if (animationNumber > animationCount)
		return;
	
	animationCurrentNumber = animationNumber;
	animationNumber--;	// Convert to array index

	//animationTimer = 0;
	animationFrame = 0;

	FRESULT fr = f_lseek(&filAnimation, animationOffset[animationNumber]);
	if (fr != FR_OK) {
		cardOK = false;
		return;
	}
	printDebug("Start animation %d (offset %ld)\n", animationNumber, animationOffset[animationNumber]);

	uint8_t fps;
    f_read(&filAnimation, &fps, sizeof(uint8_t), &br);
	animationDeltaMs = 1000 / (fps & 0x7F);	// Convert to 1ms delta
	printDebug("\tanimationDeltaMs = %u\n", animationDeltaMs);

	// MSB of FPS indicates indexed colors
	if (fps & 0x80) {
		printDebug("\tIndexed colors\n");
		uint16_t colorCount = 0;	// Must be init to 0 because f_read will only set the lower byte !

		// Number of colors is saved as index of the last color
		f_read(&filAnimation, &colorCount, sizeof(uint8_t), &br);
		colorCount++;
		printDebug("\t%d colors\n", colorCount);

		animationColorMode = colorModeFromCount(colorCount);

		animationFrameSize = byteLength(FACE_LED_STRIP_LENGTH, animationColorMode);

		// Colors won't be updated if gamma correction table changes during playback but whatever
		uint8_t colord[3] = {0,0,0};
		for (uint16_t i = 0; i < colorCount; i++) {
			f_read(&filAnimation, &colord[0], 3, &br);

			for (uint8_t j = 0; j < 3; j++)
				colorTable[i].raw[j] = gammaCorrection[colord[j]];

			printDebug("\tColor %u: R=%u G=%u B=%u\n", i, colorTable[i].rgb.r, colorTable[i].rgb.g, colorTable[i].rgb.b);
		}
		f_read(&filAnimation, &animationFrameCount, sizeof(uint16_t), &br);
		byteSwap((uint8_t *)&animationFrameCount);

	} else {
		animationColorMode = COLOR_MODE_FULL;

		animationFrameSize = FACE_LED_STRIP_LENGTH * 3;

    	f_read(&filAnimation, &animationFrameCount, sizeof(uint16_t), &br);
		byteSwap((uint8_t *)&animationFrameCount);
	}

	animationFrameStart = f_tell(&filAnimation);
	animationFrameOffset = 0;

	printDebug("\tanimationColorMode = %u\n", animationColorMode);
	printDebug("\tanimationFrameCount = %u\n", animationFrameCount);
	printDebug("\tanimationFrameStart = %u\n", animationFrameStart);
}

// Must be called every animationDeltaMs ms
void updateAnimation() {
	UINT br;

	if (animationFrame >= animationFrameCount)
		return;	// Done playing

	printDebug("animationFrame = %u\n", animationFrame);

	FRESULT fr = f_lseek(&filAnimation, animationFrameStart + animationFrameOffset);
	if (fr != FR_OK) {
		cardOK = false;
		return;
	}

	if (animationColorMode == COLOR_MODE_FULL) {
		//checkBrightness();
		//updateOSD(animationNumber + 1);
		
		uint8_t color[3];
		for (uint16_t i = 0; i < FACE_LED_STRIP_LENGTH; i++) {
    		f_read(&filAnimation, &color, 3, &br);
			for (uint8_t j = 0; j < 3; j++)
					colorBufferFace[i].raw[j] = gammaCorrection[color[j]];
		}

		refreshLEDs();

		animationFrameOffset += animationFrameSize;
	} else {
		// Indexed color

		uint8_t indexMask = (1 << animationColorMode) - 1;	// 0b0000001, 0b00000011, 0b00001111 or 0b11111111

		uint8_t colorIndexData[animationFrameSize];
		f_read(&filAnimation, &colorIndexData[0], animationFrameSize, &br);

		uint16_t ledIndex = 0;
		for (uint16_t j = 0; j < animationFrameSize; j++) {
			for (uint8_t k = 0; k < 8 / animationColorMode; k++) {
				uint8_t colorIndex = 0;

				if (j == 0 && k == 0) {
					// Very first color index of frame, get bit offset because data isn't re-aligned on bytes between each frame (TODO: get rid of this, too complicated to only save a few bits)
					uint8_t bitOffset = frameBitOffset(animationColorMode, FACE_LED_STRIP_LENGTH, animationFrame);
					colorIndex = (colorIndexData[j] >> bitOffset) & indexMask;
					k = bitOffset/animationColorMode;
				} else {
					colorIndex = (colorIndexData[j] >> (animationColorMode * k)) & indexMask;
				}
				colorBufferFace[ledIndex] = colorTable[colorIndex];

				ledIndex++;
				if (ledIndex == FACE_LED_STRIP_LENGTH)
					break;
			}
		}

		refreshLEDs();
		
		if (frameBitOffset(animationColorMode, FACE_LED_STRIP_LENGTH, animationFrame + 1) > 0)	// If there's an offset remaining, then the last byte wasn't fully consummed and must be in the next frame
			animationFrameOffset += (animationFrameSize - 1);
		else
			animationFrameOffset += animationFrameSize;
	}

	if ((animationFrame + 1) >= animationFrameCount && animationLocked) {	// && animationNumber + 1 == currentAnimation) {
		animationFrame = 0;	// Loop
		animationFrameOffset = 0;
	} else {
		animationFrame++;	// Next frame
	}
}
