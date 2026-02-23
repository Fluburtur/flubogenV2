#include <hardware/gpio.h>

/* From the FatFS library. */
#include "f_util.h"
#include "hw_config.h"
#include "sd_card.h"

#include "misc.h"
#include "sd_config.h"

// Pin       Net     Description
// GPIO6     CLK     SD card clock
// GPIO7     CMD     SD card control / DI
// GPIO8     DAT0    SD card data / DO
// GPIO9     DAT1    SD card data / x
// GPIO10    DAT2    SD card data / x
// GPIO11    DAT3    SD card data / CS

#define GPIO_PIN_CLK 6
#define GPIO_PIN_CMD 7
#define GPIO_PIN_DAT0 8
#define GPIO_PIN_DAT1 9
#define GPIO_PIN_DAT2 10
#define GPIO_PIN_DAT3 11

static sd_sdio_if_t sdio_if = {
    .CLK_gpio = GPIO_PIN_CLK,	// (D0 + SDIO_CLK_PIN_D0_OFFSET) % 32
    .CMD_gpio = GPIO_PIN_CMD,
    .D0_gpio = GPIO_PIN_DAT0,
    .D1_gpio = GPIO_PIN_DAT1,
    .D2_gpio = GPIO_PIN_DAT2,
    .D3_gpio = GPIO_PIN_DAT3,
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
    .baud_rate = 50 * 1000 * 1000 / 12  // 4.16 MHz  TODO: should be able to go faster
};

// Hardware Configuration of the SD Card socket "object"
static sd_card_t sd_card = {
    .type = SD_IF_SDIO,
    .sdio_if_p = &sdio_if,
    // SD Card detect:
    .use_card_detect = false
};

static FATFS fs;
static bool is_card_ok;

/*
 * Technically public, but only used by the FatFS library. Used to configure the card.
 */

size_t sd_get_num() {
    return 1;
}

sd_card_t* sd_get_by_num(size_t num) {
    if (0 == num)
        return &sd_card;
    else
        return NULL;
}

/*
 * The user-facing functions.
 */

void sd_config_init(void)
{
    /* No need to configure the GPIO pins ourself, the SD card library will do
     * it for us. */

    FRESULT fr = f_mount(&fs, "", 1);
    if (FR_OK != fr)
    {
        printDebug("f_mount failed: %s (%d)\n", FRESULT_str(fr), fr);
        is_card_ok = false;
    }
    else
    {
        is_card_ok = true;
    }
}

bool sd_config_is_card_ok(void)
{
    return is_card_ok;
}
