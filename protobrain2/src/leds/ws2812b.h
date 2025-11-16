#ifndef _WS2812B_H_
#define _WS2812B_H_

#include <stdint.h>

#include <hardware/pio.h>

typedef union
{
    uint32_t grbx;
    // Little endian!
    struct
    {
        uint8_t _x;
        uint8_t b;
        uint8_t r;
        uint8_t g;
    };
} wsb2812b_led_value_t;

typedef struct
{
    uint gpio_pin;
    uint num_leds;
} ws2812b_channel_init_t;

typedef struct
{
    PIO pio_instance;
    uint sm_instance;
    uint dma_channel;
    uint num_leds;
} ws2812b_channel_t;


void ws2812b_init(
    const ws2812b_channel_init_t *channel_requests, ws2812b_channel_t *channel_results,
    uint num_channels);
void ws2812b_wait_for_idle(const ws2812b_channel_t *channel);
void ws2812b_send_buffer(const ws2812b_channel_t *channel, const ws2812b_led_value_t *values);

#endif /* #ifndef _WS2812B_H_ */
