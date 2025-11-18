#ifndef _MISC_H_
#define _MISC_H_

#include <stdbool.h>
#include <stdint.h>

void byteSwap(uint8_t *x);
//void printBits(size_t const size, void const * const ptr);
void printDebug(const char *format, ...);
void enablePrintDebug(bool enable);

#endif /* _MISC_H_ */
