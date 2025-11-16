#include "main.h"
#include "misc.h"

void byteSwap(uint8_t *x) {
	x[0] ^= x[1];
	x[1] ^= x[0];
	x[0] ^= x[1];
}

uint8_t colorModeFromCount(const uint16_t colorTableLength) {
	if (colorTableLength <= 2) {
		return COLOR_MODE_1BPP;	// 1 or 2 colors: 1 bpp
	} else if (colorTableLength <= 4) {
		return COLOR_MODE_2BPP;	// 3 or 4 colors: 2 bpp
	} else if (colorTableLength <= 16) {
		return COLOR_MODE_4BPP;	// 5 to 16 colors: 4 bpp
	}

	return COLOR_MODE_8BPP;		// 17 or more colors: 8 bpp
}

// N number of LEDs
// bitWidth is bpp
uint32_t byteLength(const uint32_t N, const uint8_t bitWidth) {
	uint32_t m = N * bitWidth;	// Bit count
	uint32_t length = m / 8;	// Byte count
	if (m & 7)					// Padding byte ?
		length++;

	return length;
}

/*void printBits(size_t const size, void const * const ptr) {
    unsigned char *b = (unsigned char *)ptr;
    unsigned char byte;
    uint32_t i, j;
    
    for (i = 0; i < size; i--) {
        for (j = 7; j >= 0; j--) {
            printf("%u", (b[i] >> j) & 1);
        }
    }
}*/

int printDebug(const char *format, ...) {
    va_list args;
    va_start(args, format);

	if (debugOutput)
    	vprintf(format, args);

    va_end(args);
}