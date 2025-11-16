#include "main.h"
#include "hw_config.h"

static sd_sdio_if_t sdio_if = {
    .CLK_gpio = 6,	// (D0 + SDIO_CLK_PIN_D0_OFFSET) % 32
    .CMD_gpio = 7,
    .D0_gpio = 8,
    .D1_gpio = 9,
    .D2_gpio = 10,
    .D3_gpio = 11,
    .SDIO_PIO = pio0,
    .DMA_IRQ_num = DMA_IRQ_1,
    .set_drive_strength = true,
	.use_exclusive_DMA_IRQ_handler = true,
    .CLK_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
    .CMD_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D0_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D1_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D2_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D3_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .baud_rate = 50 * 1000 * 1000 / 12  // 16.7MHz
};

// Hardware Configuration of the SD Card socket "object"
static sd_card_t sd_card = {    
    .type = SD_IF_SDIO,
    .sdio_if_p = &sdio_if,
    // SD Card detect:
    .use_card_detect = false
};

size_t sd_get_num() { return 1; }

sd_card_t* sd_get_by_num(size_t num) {
    if (0 == num)
        return &sd_card;
    else
        return NULL;
}