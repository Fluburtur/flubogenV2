#ifndef _ANIM_H_
#define _ANIM_H_

#include <stdbool.h>
#include <stdint.h>

/* These are for the new .fur format, currently unused. */
#if 0

typedef struct {
    uint8_t x;
    uint8_t y;
} led_coord_t;

// New .fur format stuff
typedef struct {
    char magic[3];
    uint8_t ver;

    uint8_t output;
    uint16_t led_count;
    //led_coord_t* leds;
} fur_header_t;

typedef struct {
    uint8_t anim_count;
    //fur_anim_t* anims;
} fur_animlist_t;

typedef struct {
    char name[16];
    uint16_t frame_count;
    uint8_t color_format;
} fur_anim_t;

typedef struct {
    uint8_t color_count;
    //rgbColor24bpp_t* colors;
} fur_palette_t;

#endif

typedef enum {
	NO_ANIMATION,
	DEFAULT_ANIMATION,
	RANDOM_ANIMATION_1,
	RANDOM_ANIMATION_2,
	RANDOM_ANIMATION_3,
	BOOT_ANIMATION
} animationType_t;

#define RGB_COLOR_COMPONENT_COUNT 3

struct rgbColor24bpp_s {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef union {
    // In memory, should be: r, g, b
    // Read as LE uint32_t: x, b, g, r
    struct rgbColor24bpp_s rgb;
    uint8_t raw[RGB_COLOR_COMPONENT_COUNT];
    uint32_t rawlong;
} rgbColor24bpp_t;

extern const char *ANIMATION_NAME[];

bool animationInit(void);
void startAnimation(uint8_t animationNumber);
void updateAnimation();

#endif /* _ANIM_H_ */
