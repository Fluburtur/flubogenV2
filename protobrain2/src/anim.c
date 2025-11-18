#include <string.h>
#include <stdlib.h>

#include "anim.h"
#include "ff.h"
#include "leds/leds.h"
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

/* The .fur file has the animation's RGB values in the range 0-255.
 * We want to map these input values to different output values for the LEDs. Because:
 *   - Limit max brightness so we don't draw too much current.
 *   - Limit max brightness so we don't blind people in dark rooms.
 *   - A linear input to a non-linear output, for gamma correction.
 *
 * For now, we just do a fixed linear brightness limit to protect the power supply.
 * TODO: variable brightness limit and gamma correction. */
static const uint8_t brightnessMap[256] = {
	0, 1, 1, 2, 2, 3, 4, 4,
	5, 5, 6, 6, 7, 8, 8, 9,
	9, 10, 11, 11, 12, 12, 13, 14,
	14, 15, 15, 16, 16, 17, 18, 18,
	19, 19, 20, 21, 21, 22, 22, 23,
	24, 24, 25, 25, 26, 26, 27, 28,
	28, 29, 29, 30, 31, 31, 32, 32,
	33, 34, 34, 35, 35, 36, 36, 37,
	38, 38, 39, 39, 40, 41, 41, 42,
	42, 43, 44, 44, 45, 45, 46, 46,
	47, 48, 48, 49, 49, 50, 51, 51,
	52, 52, 53, 54, 54, 55, 55, 56,
	56, 57, 58, 58, 59, 59, 60, 61,
	61, 62, 62, 63, 64, 64, 65, 65,
	66, 66, 67, 68, 68, 69, 69, 70,
	71, 71, 72, 72, 73, 74, 74, 75,
	75, 76, 76, 77, 78, 78, 79, 79,
	80, 81, 81, 82, 82, 83, 84, 84,
	85, 85, 86, 86, 87, 88, 88, 89,
	89, 90, 91, 91, 92, 92, 93, 94,
	94, 95, 95, 96, 96, 97, 98, 98,
	99, 99, 100, 101, 101, 102, 102, 103,
	104, 104, 105, 105, 106, 106, 107, 108,
	108, 109, 109, 110, 111, 111, 112, 112,
	113, 114, 114, 115, 115, 116, 116, 117,
	118, 118, 119, 119, 120, 121, 121, 122,
	122, 123, 124, 124, 125, 125, 126, 126,
	127, 128, 128, 129, 129, 130, 131, 131,
	132, 132, 133, 134, 134, 135, 135, 136,
	136, 137, 138, 138, 139, 139, 140, 141,
	141, 142, 142, 143, 144, 144, 145, 145,
	146, 146, 147, 148, 148, 149, 149, 150,
};

#define FILE_NAME "file.fur"

/* .fur file handle */
static FIL filAnimation;
/* Total number of animations in the file. */
static uint8_t animationCount;
/* Byte offset of the animation within the file. */
static int32_t animationOffset[256];
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

			offset += byteLength(frameCount * NUM_LEDS_FACE, colorModeFromCount(colorCount));
		} else {
			printDebug("\tDirect colors\n");

    		f_read(&filAnimation, &frameCount, sizeof(uint16_t), &br);
			byteSwap((uint8_t*)&frameCount);
			printDebug("\t%d frames\n", frameCount);

			offset += (2 + 3 * frameCount * NUM_LEDS_FACE);
		}
	}

	return true;
}

void startAnimation(uint8_t animationNumber) {
	/* Number of bytes actually read. */
	UINT br;

	if (animationNumber == NO_ANIMATION) {
		// Clear
		ws2812b_led_value_t off = {.grbx = 0};
		leds_set_channel_to_colour(LED_CHANNEL_FACE, off, false);
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

		animationFrameSize = byteLength(NUM_LEDS_FACE, animationColorMode);

		// Colors won't be updated if gamma correction table changes during playback but whatever
		uint8_t colord[3] = {0,0,0};
		for (uint16_t i = 0; i < colorCount; i++) {
			f_read(&filAnimation, &colord[0], 3, &br);

			for (uint8_t j = 0; j < 3; j++) {
				colorTable[i].raw[j] = brightnessMap[colord[j]];
			}

			printDebug("\tColor %u: R=%u G=%u B=%u\n", i, colorTable[i].rgb.r, colorTable[i].rgb.g, colorTable[i].rgb.b);
		}
		f_read(&filAnimation, &animationFrameCount, sizeof(uint16_t), &br);
		byteSwap((uint8_t *)&animationFrameCount);

	} else {
		animationColorMode = COLOR_MODE_FULL;

		animationFrameSize = NUM_LEDS_FACE * 3;

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

	ws2812b_led_value_t *colorBufferFace = leds_get_buffer_for_channel(LED_CHANNEL_FACE);

	if (animationColorMode == COLOR_MODE_FULL) {
		//checkBrightness();
		//updateOSD(animationNumber + 1);

		uint8_t color[3];
		for (uint16_t i = 0; i < NUM_LEDS_FACE; i++) {
    		f_read(&filAnimation, &color, 3, &br);

			/* The color is stored in the file as RGB bytes. */
			colorBufferFace[i].r = brightnessMap[color[0]];
			colorBufferFace[i].g = brightnessMap[color[1]];
			colorBufferFace[i].b = brightnessMap[color[2]];
		}

		leds_write_buffer_to_channel(LED_CHANNEL_FACE);

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
					uint8_t bitOffset = frameBitOffset(animationColorMode, NUM_LEDS_FACE, animationFrame);
					colorIndex = (colorIndexData[j] >> bitOffset) & indexMask;
					k = bitOffset/animationColorMode;
				} else {
					colorIndex = (colorIndexData[j] >> (animationColorMode * k)) & indexMask;
				}

				/* The color is stored in the table as RGB bytes. */
				rgbColor24bpp_t color = colorTable[colorIndex];
				colorBufferFace[ledIndex].r = color.rgb.r;
				colorBufferFace[ledIndex].g = color.rgb.g;
				colorBufferFace[ledIndex].b = color.rgb.b;

				ledIndex++;
				if (ledIndex == NUM_LEDS_FACE)
					break;
			}
		}

		leds_write_buffer_to_channel(LED_CHANNEL_FACE);

		/* If there's an offset remaining, then the last byte wasn't fully consummed and must be in
		 * the next frame. */
		if (frameBitOffset(animationColorMode, NUM_LEDS_FACE, animationFrame + 1) > 0)
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

void animationSetLocked(bool locked)
{
	animationLocked = locked;
}
