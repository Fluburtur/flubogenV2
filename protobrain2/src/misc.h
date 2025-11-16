#ifndef _MISC_H_
#define _MISC_H_

void byteSwap(uint8_t *x);
uint8_t colorModeFromCount(const uint16_t colorTableLength);
uint32_t byteLength(const uint32_t N, const uint8_t bitWidth);
//void printBits(size_t const size, void const * const ptr);
int printDebug(const char *format, ...);

#endif /* _MISC_H_ */
