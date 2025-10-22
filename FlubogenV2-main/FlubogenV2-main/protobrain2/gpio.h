#if !defined(_GPIO_H_)
#define _GPIO_H_

#include "main.h"

#define PULL_NONE 0
#define STATE_LOW 0
#define PULL_UP 1
#define PULL_DOWN 2
#define STATE_HIGH 4

typedef struct {
	uint8_t number;
	uint8_t direction;
	uint8_t function;
	uint8_t pulls_state;
} iodef_t;

extern iodef_t iodefs[23];

#endif /* _GPIO_H_ */
