#include <stdint.h>

#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <pico/assert.h>
#include <pico/stdlib.h>

#include "ws2812b.h"
#include "ws2812b.pio.h"

#define PINDIR_OUTPUT true
#define OSR_SHIFT_LEFT false
#define AUTOPULL_YES true
#define BITS_PER_LED 24
// One bit takes 1.25 us == 800 KHz
#define BAUD_RATE 800000

// Uncomment to slow down the LED by 8x for a slow logic analyser.
//#define SLOW_LOGIC_ANALYSER

static void setup_pio(
    PIO pio_instance, uint sm_instance, uint *dma_channel, uint program_offset, uint gpio_pin,
    uint num_leds);

void ws2812b_init(
    const ws2812b_channel_init_t *channel_requests, ws2812b_channel_t *channel_results,
    uint num_channels)
{
    PIO pio_instance;
    bool need_new_pio = true;
    uint program_offset;

    for (uint i = 0; i < num_channels; i++)
    {
        uint sm_instance;
        uint dma_channel;

        // We'll try to use a SM in the existing PIO instance.
        if (!need_new_pio)
        {
            int maybe_sm_instance = pio_claim_unused_sm(pio_instance, false);
            if (maybe_sm_instance >= 0)
            {
                sm_instance = (uint)maybe_sm_instance;
            }
            else
            {
                need_new_pio = true;
            }
        }

        // We need a new PIO instance if this is the first channel, or if the existing PIO instance
        // has no free SMs.
        if (need_new_pio)
        {
            bool success = pio_claim_free_sm_and_add_program(
                &ws2812b_program, &pio_instance, &sm_instance, &program_offset);
            hard_assert(success);
            need_new_pio = false;
        }

        setup_pio(
            pio_instance, sm_instance, &dma_channel, program_offset, channel_requests[i].gpio_pin,
            channel_requests[i].num_leds);

        channel_results[i].pio_instance = pio_instance;
        channel_results[i].sm_instance = sm_instance;
        channel_results[i].dma_channel = dma_channel;
        channel_results[i].num_leds = channel_requests[i].num_leds;
    }
}

void ws2812b_wait_for_idle(const ws2812b_channel_t *channel)
{
    bool empty = false;
    while (!empty)
    {
        empty = pio_sm_is_tx_fifo_empty(channel->pio_instance, channel->sm_instance);
    }
}

void ws2812b_send_buffer(const ws2812b_channel_t *channel, const ws2812b_led_value_t *values)
{
    dma_channel_set_read_addr(channel->dma_channel, values, true);
}

/** Set up the PIO SM and DMA channel for one display. */
static void setup_pio(
    PIO pio_instance, uint sm_instance, uint *dma_channel, uint program_offset, uint gpio_pin, uint num_leds)
{
    int result;

    pio_gpio_init(pio_instance, gpio_pin);
    result = pio_sm_set_consecutive_pindirs(pio_instance, sm_instance, gpio_pin, 1, PINDIR_OUTPUT);
    hard_assert(result == PICO_OK);
    pio_sm_config sm_config = ws2812b_program_get_default_config(program_offset);
    sm_config_set_sideset_pins(&sm_config, gpio_pin);
    sm_config_set_out_shift(&sm_config, OSR_SHIFT_LEFT, AUTOPULL_YES, BITS_PER_LED);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);

    int cycles_per_bit = ws2812b_T1 + ws2812b_T2 + ws2812b_T3;
    float div = clock_get_hz(clk_sys) / (BAUD_RATE * cycles_per_bit);
#ifdef SLOW_LOGIC_ANALYSER
    #pragma message "Warning: WS2812B driver is at 1/8 speed for debugging!"
    div *= 8;
#endif
    sm_config_set_clkdiv(&sm_config, div);
    result = pio_sm_init(pio_instance, sm_instance, program_offset, &sm_config);
    hard_assert(result == PICO_OK);
    pio_sm_set_enabled(pio_instance, sm_instance, true);

    uint dma_chan = (uint)dma_claim_unused_channel(true);
    dma_channel_config_t dma_cfg = {0};
    channel_config_set_read_increment(&dma_cfg, true);
    channel_config_set_write_increment(&dma_cfg, false);
    channel_config_set_dreq(&dma_cfg, pio_get_dreq(pio_instance, sm_instance, true));
    channel_config_set_chain_to(&dma_cfg, dma_chan); /* No chaining */
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
    channel_config_set_ring(&dma_cfg, false, 0); /* Ring buffer off */
    channel_config_set_bswap(&dma_cfg, false);
    channel_config_set_irq_quiet(&dma_cfg, false);
    channel_config_set_enable(&dma_cfg, true);
    channel_config_set_sniff_enable(&dma_cfg, false); /* CRC? No */
    channel_config_set_high_priority(&dma_cfg, false);
    // Don't trigger anything yet when setting these.
    dma_channel_set_config(dma_chan, &dma_cfg, false);
    dma_channel_set_write_addr(dma_chan, &pio_instance->txf[sm_instance], false);
    dma_channel_set_transfer_count(dma_chan, dma_encode_transfer_count(num_leds), false);

    *dma_channel = dma_chan;
}
