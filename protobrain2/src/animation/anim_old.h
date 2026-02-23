#ifndef _ANIM_H_
#define _ANIM_H_

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

typedef enum {
	COLOR_MODE_FULL = 0,    // 24bpp
	COLOR_MODE_1BPP = 1,    // 1bpp
	COLOR_MODE_2BPP = 2,
	COLOR_MODE_4BPP = 4,
	COLOR_MODE_8BPP = 8
} color_mode_t;

extern uint16_t animationDeltaMs;
extern const char *ANIMATION_NAME[];

extern uint16_t animationFrameCount;
extern uint32_t animationFrame;
extern uint8_t animationCurrentNumber;

bool animationInit(void);
void startAnimation(uint8_t animationNumber);
void updateAnimation();

#endif /* _ANIM_H_ */
