#include "main.h"
#include "misc.h"

void byteSwap(uint8_t *x) {
	x[0] ^= x[1];
	x[1] ^= x[0];
	x[0] ^= x[1];
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
