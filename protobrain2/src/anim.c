#include <string.h>
#include <stdlib.h>

#include "anim.h"
#include "ff.h"
#include "misc.h"

typedef enum {
	/* 24 bpp */
	COLOR_MODE_FULL = 0,
	COLOR_MODE_1BPP = 1,
	COLOR_MODE_2BPP = 2,
	COLOR_MODE_4BPP = 4,
	COLOR_MODE_8BPP = 8
} color_mode_t;

const char *ANIMATION_NAME[] = {
	"NO ANIMATION",
	"DEFAULT ANIMATION",
	"RANDOM ANIMATION 1",
	"RANDOM ANIMATION 2",
	"RANDOM ANIMATION 3",
	"BOOT ANIMATION"
};

#define FILE_NAME "file.fur"

/* .fur file handle */
static FIL filAnimation;
/* Total number of animations in the file. */
static uint8_t animationCount;
/* Byte offset of the animation within the file. */
static int32_t animationOffset[256];
static rgbColor24bpp_t colorBufferFace[FACE_LED_STRIP_LENGTH];
static uint16_t animationDeltaMs;
/* The color mode used for this animation. */
static color_mode_t animationColorMode;
/* Position in the file of the first frame's data. */
static uint32_t animationFrameDataStart;
/* Offset in the file from `animationFrameDataStart` for the current frame's data. */
static uint32_t animationFrameDataOffset;
/* How many bytes in one animation frame. */
static uint32_t animationFrameSize;
/* How many frames in the currently playing animation. */
static uint16_t animationFrameCount;
/* The frame number within the currently playing animation. */
static uint32_t animationFrame;
/* The one-based animation number of the current animation. */
static uint8_t animationCurrentNumber;
/* If true, the animation loops when it ends. */
static bool animationLocked;
/* For indexed color mode; stores the color value for each index. */
static rgbColor24bpp_t colorTable[256];

// bitWidth is bpp
static uint32_t byteLength(uint32_t num_leds, uint8_t bitWidth) {
	uint32_t num_bits = num_leds * bitWidth;
	uint32_t num_bytes = num_bits / 8;
	/* We can't read partial bytes; read a whole byte when that happens. */
	if (num_bits & 7)
	{
		num_bytes++;
	}

	return num_bytes;
}

static uint8_t colorModeFromCount(uint16_t colorTableLength) {
	if (colorTableLength <= 2) {
		return COLOR_MODE_1BPP;	// 1 or 2 colors: 1 bpp
	} else if (colorTableLength <= 4) {
		return COLOR_MODE_2BPP;	// 3 or 4 colors: 2 bpp
	} else if (colorTableLength <= 16) {
		return COLOR_MODE_4BPP;	// 5 to 16 colors: 4 bpp
	}

	return COLOR_MODE_8BPP;		// 17 or more colors: 8 bpp
}

uint8_t frameBitOffset(const uint8_t bitWidth, const uint32_t pixelsPerFame, const uint32_t frame) {
	return (bitWidth * pixelsPerFame * (frame & 7)) & 7;
}

// This only gets a list of offsets to the start of each animation data in the FUR file
bool animationInit(void) {
	/* Number of bytes actually read. */
	UINT br;
	uint32_t offset = 1; // Start after animation count byte
	uint16_t frameCount;

	FRESULT fileOpened = f_open(&filAnimation, FILE_NAME, FA_READ);
	printDebug("File opened: %d\n", fileOpened);

	if (fileOpened != FR_OK)
	{
		return false;
	}

    f_read(&filAnimation, &animationCount, sizeof(uint8_t), &br);
	printDebug("Number of animations: %d\n", animationCount);

	for (uint8_t i = 0; i < animationCount; i++) {
		animationOffset[i] = offset;
		f_lseek(&filAnimation, offset);
		printDebug("Animation %d has offset %ld\n", i, animationOffset[i]);

		uint8_t fps_and_color_option;
    	f_read(&filAnimation, &fps_and_color_option, sizeof(uint8_t), &br);
		uint8_t fps = (fps_and_color_option & 0x7F);
		printDebug("\t%d FPS\n", fps);
		offset++;

        // MSB of FPS indicates indexed colors
		if (fps_and_color_option & 0x80) {
			printDebug("\tIndexed colors\n");
			uint16_t colorCount = 0;

			// Number of colors is saved as index of the last color
    		f_read(&filAnimation, &colorCount, sizeof(uint8_t), &br);
			offset++;
			colorCount++;
			printDebug("\t%d colors\n", colorCount);

            // Skip over color table
			f_lseek(&filAnimation, offset + (3 * colorCount));
			offset += (3 * colorCount);

			// Number of frames in animation
    		f_read(&filAnimation, &frameCount, sizeof(uint16_t), &br);
			offset += 2;
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

void startAnimation(uint8_t animationNumber) {
	/* Number of bytes actually read. */
	UINT br;

	if (animationNumber == NO_ANIMATION) {
		// Clear
		memset(colorBufferFace, 0, sizeof(rgbColor24bpp_t) * FACE_LED_STRIP_LENGTH);
		refreshLEDs(LEDS_FACE_CHEEK);
		return;
	}

	if (animationNumber > animationCount)
	{
		return;
	}

	animationCurrentNumber = animationNumber;
	uint8_t animationIndex = animationNumber - 1;

	animationFrame = 0;

	FRESULT fr = f_lseek(&filAnimation, animationOffset[animationIndex]);
	if (fr != FR_OK) {
		/* TODO: restore card failure reporting. */
		//cardOK = false;
		return;
	}
	printDebug("Start animation %d (offset %ld)\n", animationIndex, animationOffset[animationIndex]);

	uint8_t fps_and_color_option;
    f_read(&filAnimation, &fps_and_color_option, sizeof(uint8_t), &br);
	uint8_t fps = (fps_and_color_option & 0x7F);
	animationDeltaMs = 1000 / fps;	// Convert to 1ms delta
	printDebug("\tanimationDeltaMs = %u\n", animationDeltaMs);

	// MSB of FPS indicates indexed colors
	if (fps_and_color_option & 0x80) {
		printDebug("\tIndexed colors\n");
		/* How many colors are used in this animation. Can reach 256.
		 * Must be init to 0 because f_read will only set the lower byte! */
		uint16_t colorCount = 0;

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

			for (uint8_t j = 0; j < 3; j++) {
				/* TODO: restore the gamma correction functionality. */
				//colorTable[i].raw[j] = gammaCorrection[colord[j]];
				colorTable[i].raw[j] = colord[j];
			}

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

	animationFrameDataStart = f_tell(&filAnimation);
	animationFrameDataOffset = 0;

	printDebug("\tanimationColorMode = %u\n", animationColorMode);
	printDebug("\tanimationFrameCount = %u\n", animationFrameCount);
	printDebug("\tanimationFrameDataStart = %u\n", animationFrameDataStart);
}

// Must be called every animationDeltaMs ms
void updateAnimation() {
	/* Number of bytes actually read. */
	UINT br;

	if (animationFrame >= animationFrameCount)
		return;	// Done playing

	printDebug("animationFrame = %u\n", animationFrame);

	FRESULT fr = f_lseek(&filAnimation, animationFrameDataStart + animationFrameDataOffset);
	if (fr != FR_OK) {
		/* TODO: restore card failure reporting. */
		//cardOK = false;
		return;
	}

	if (animationColorMode == COLOR_MODE_FULL) {
		//checkBrightness();
		//updateOSD(animationNumber + 1);

		uint8_t color[3];
		for (uint16_t i = 0; i < FACE_LED_STRIP_LENGTH; i++) {
    		f_read(&filAnimation, &color, 3, &br);
			for (uint8_t j = 0; j < 3; j++) {
				/* TODO: restore the gamma correction functionality. */
				//colorBufferFace[i].raw[j] = gammaCorrection[color[j]];
				colorBufferFace[i].raw[j] = color[j];
			}
		}

		refreshLEDs();

		animationFrameDataOffset += animationFrameSize;
	} else {
		// Indexed color

		/* 0b0000001, 0b00000011, 0b00001111 or 0b11111111 */
		uint8_t indexMask = (1 << animationColorMode) - 1;

		uint8_t colorIndexData[animationFrameSize];
		f_read(&filAnimation, &colorIndexData[0], animationFrameSize, &br);

		uint16_t ledIndex = 0;
		/* For all of the bytes of animation data... */
		for (uint16_t j = 0; j < animationFrameSize; j++) {
			/* Process the colours (1, 2, or 4 per byte) */
			for (uint8_t k = 0; k < 8 / animationColorMode; k++) {
				uint8_t colorIndex = 0;

				if (j == 0 && k == 0) {
					/* Very first color index of frame, get bit offset because data isn't re-aligned
					 * on bytes between each frame (TODO: get rid of this, too complicated to only
					 * save a few bits). */
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

		/* If there's an offset remaining, then the last byte wasn't fully consummed and must be in
		 * the next frame. */
		if (frameBitOffset(animationColorMode, FACE_LED_STRIP_LENGTH, animationFrame + 1) > 0)
			animationFrameDataOffset += (animationFrameSize - 1);
		else
			animationFrameDataOffset += animationFrameSize;
	}

	/* If we've reached the end of the animation and we're locked, repeat the animation. */
	if (animationLocked && ((animationFrame + 1) >= animationFrameCount)) {	// && animationNumber + 1 == currentAnimation) {
		animationFrame = 0;	// Loop
		animationFrameDataOffset = 0;
	} else {
		animationFrame++;	// Next frame
	}
}
