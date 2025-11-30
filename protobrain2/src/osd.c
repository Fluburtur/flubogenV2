#include <stdint.h>

#include <hardware/gpio.h>
#include <hardware/spi.h>

#include "osd.h"

#define GPIO_PIN_OSD_CIPO 16
#define GPIO_PIN_OSD_NCS 17
#define GPIO_PIN_OSD_SCK 18
#define GPIO_PIN_OSD_COPI 19

#define MAX7456_READ 0x80
#define MAX7456_VM0 0x00
#define MAX7456_VM1 0x01
#define MAX7456_HOS 0x02
#define MAX7456_VOS 0x03
#define MAX7456_DMM 0x04
#define MAX7456_DMAH 0x05
#define MAX7456_DMAL 0x06
#define MAX7456_DMDI 0x07
#define MAX7456_CMM 0x08
#define MAX7456_CMAH 0x09
#define MAX7456_CMAL 0x0a
#define MAX7456_CMDI 0x0b
#define MAX7456_OSDM 0x0c
#define MAX7456_RB0 0x10
#define MAX7456_RB1 0x11
#define MAX7456_RB2 0x12
#define MAX7456_RB3 0x13
#define MAX7456_RB4 0x14
#define MAX7456_RB5 0x15
#define MAX7456_RB6 0x16
#define MAX7456_RB7 0x17
#define MAX7456_RB8 0x18
#define MAX7456_RB9 0x19
#define MAX7456_RB10 0x1a
#define MAX7456_RB11 0x1b
#define MAX7456_RB12 0x1c
#define MAX7456_RB13 0x1d
#define MAX7456_RB14 0x1e
#define MAX7456_RB15 0x1f
#define MAX7456_OSDBL 0x6c
#define MAX7456_STAT 0xA0
#define MAX7456_CMDO 0xC0
#define MAX7456_PAL_ROWS 16

static const uint8_t defaultCodePage[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x46,
    0x3F, 0x40, 0x00, 0x00, 0x45, 0x49, 0x41, 0x47,
    0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x44, 0x43, 0x4A, 0x00, 0x4B, 0x42,
    0x4C, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
    0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint16_t osd_buffer[480]; // 30 chars * 16 rows

static void busWrite(uint8_t address, uint8_t data)
{
    spi_get_hw(spi0)->dr = (uint32_t)((address << 8) | data);
    while (spi_is_busy(spi0))
    {
    };

    while (spi_is_readable(spi0))
        (void)spi_get_hw(spi0)->dr;
}

static uint16_t busRead(uint8_t address)
{
    spi_get_hw(spi0)->dr = (address | 0x80) << 8;
    while (spi_is_busy(spi0))
    {
    };

    return (uint16_t)spi_get_hw(spi0)->dr;
}

static void osd_print(uint8_t x, uint8_t y, char const *str, uint8_t attr)
{
    char c;
    uint16_t addr;

    addr = y * 30 + x;
    attr = (attr & 3) << 5;

    while ((c = *str++))
    {
        c = defaultCodePage[c & 127];
        busWrite(MAX7456_DMAH, addr >> 8);
        busWrite(MAX7456_DMAL, addr);
        busWrite(MAX7456_DMDI, c);
        busWrite(MAX7456_DMAH, (addr >> 8) + 2);
        busWrite(MAX7456_DMAL, addr);
        busWrite(MAX7456_DMDI, attr);
        addr++;
    }
}

void osd_init(void)
{
    // Init SPI0 for MAX7456
    gpio_set_function(GPIO_PIN_OSD_CIPO, GPIO_FUNC_SPI);
    gpio_set_function(GPIO_PIN_OSD_NCS, GPIO_FUNC_SPI);
    gpio_set_function(GPIO_PIN_OSD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(GPIO_PIN_OSD_COPI, GPIO_FUNC_SPI);
    spi_init(spi0, 5 * 1000 * 1000); // 50M / 10 = 5MHz, MAX7456 supports up to 10MHz
    spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // spi_write_blocking(spi0, )

    // Wait for POR done
    // while(busRead(MAX7456_STAT) & 0x40);

    // Force software reset
    busWrite(MAX7456_VM0, 0x42);

    // Set screen buffer to all blanks
    for (uint32_t c = 0; c < 480; c++)
    {
        osd_buffer[c] = 0;
    }

    // Wait for software reset done
    while (busRead(MAX7456_VM0) & 0x02)
    {
    };

    // busWrite(MAX7456_VM0, 0b00100000);	// NTSC external sync, output off
    busWrite(MAX7456_VM0, 0b00110000); // 0b01001000 PAL auto sync, OSD on ---- orginal: 0x48
    // Set all rows to same black/white level
    for (uint32_t c = 0; c < 16; c++)
    {
        busWrite(MAX7456_RB0 + c, 0b00000001); // 0% black, 100% white
                                               // busWrite(MAX7456_RB0 + c, 0b00001010);	// 0% black, 100% white
    }
    // busWrite(MAX7456_VM1, 0b11011100);	// Background 35% grey, 133ms blinking 50%
    busWrite(MAX7456_VM1, 0b01001000); // Background 0% grey, 133ms blinking 50%

    busWrite(MAX7456_DMM, 0b01000000);                       // Unique attributes mode, no auto-inc
    uint8_t factory_OSDBL = (uint8_t)busRead(MAX7456_OSDBL); // must read OSDBL first and rewrite factory values
    busWrite(MAX7456_OSDBL, factory_OSDBL & 0b11101111);
    // osd_print(12, 5, str_a, 3);

    // Display memory 0~511 = char, 512~1023 = attribute
    // Write to DMDI
    // Attribute byte: LBC, BLK, INV, 00000

    busWrite(MAX7456_VM0, 0b01001000); // 0b01001000 PAL auto sync, OSD on ---- 0x48
}

void updateOSD(uint8_t animationNumber)
{
#if 1
    /* The code that was in main_old.c when I started to overhaul everything. Seems to be an
     * attempt to port from the old STM32 code to the RP2040. */

    // printf("MAX7456_VM0: %02X\n", busRead(MAX7456_VM0));
    //  osd_print(2, 1, "DEBUG MODE", 1);

    // // 10k/1k voltage divider
    uint32_t result = (ADCAvgBattV * 9075) >> 10; // * 11 * 3.3 * 1000 / 4096: mV (22.10)
    sprintf(strBuffer, "%2u.%03uV", result / 1000, result % 1000);
    osd_print(1, MAX7456_PAL_ROWS - 2, strBuffer, 0);

    // // Vo = IS RS RL / 5k = IS * 0.25 * 14300 / 5000 = IS * 0.715
    // // IS = Vo / (0.25 * 14300 / 5000) = Vo / 0.715 = Vo * 1.399
    // // Pico ADC precision is crap, low current won't be accurately measured (60mA real shows 130mA)
    // result = (ADCAvgCurrent * 1154) >> 10;	// * 1.399 * 3.3 * 1000 / 4096: mA (22.10)
    // sprintf(strBuffer, "CURNT:%04u (%1u.%03uA)", ADCAvgCurrent, result / 1000, result % 1000);
    // osd_print(2, 3, strBuffer, 0);

    // result = ADCAvgBrightness;
    // sprintf(strBuffer, "BRGHT:%04u", ADCAvgBrightness);
    // osd_print(2, 4, strBuffer, 0);

    // sprintf(strBuffer, "REMOTE:%s", remoteOK ? "OK  " : "FAIL");
    // osd_print(2, 5, strBuffer, 0);
    // unsigned char * ptr = &strBuffer[0];
    // // Binary display
    // for (uint32_t i = 0; i < 2; i++) {
    // 	for (uint32_t j = 0; j < 8; j++) {
    // 		*(ptr++) = ('0' + ((lastRemoteData[i] << j) & 0x80));
    // 	}
    // 	*(ptr++) = ' ';
    // }
    // *(ptr++) = 0;
    // osd_print(9, 6, strBuffer, 0);

    // sprintf(strBuffer, "CARD:%s", cardOK ? "OK  " : "FAIL");
    // osd_print(2, 7, strBuffer, 0);

    osd_print(1, MAX7456_PAL_ROWS - 3, ANIMATION_NAME[animationCurrentNumber], 0);
    msSinceBoot = to_ms_since_boot(get_absolute_time());
    seconds = (msSinceBoot / 1000) % 60;
    minutes = (msSinceBoot / (1000 * 60)) % 60;
    hours = (msSinceBoot / (1000 * 60 * 60)) % 60;
    sprintf(strBuffer, "%d:%d:%d", hours, minutes, seconds);
    osd_print(20, MAX7456_PAL_ROWS - 2, strBuffer, 0);

    sprintf(strBuffer, "remote:%d", lastRemoteData[0]);
    osd_print(1, MAX7456_PAL_ROWS - 4, strBuffer, 0);

#else
    /* The code that was in osd.c updateOSD() when I started to overhaul everything. Seems to be
     * a partly annotated version of the old STM32 code. */

    // For some reason the OSD can only display 28 characters on one line instead of the 30 that is specified in datasheet
    char lineBuffer[(MAX7456_CHARS_PER_LINE - 2) + 1];

    LOG_E(SYSTEM, "ADC_CURRENT: %d", adcGetChannel(ADC_CURRENT));

    timeUs_t newBatteryUpdate = micros();
    timeUs_t delta = newBatteryUpdate - lastBatteryUpdate;

    // Update battery measurements
    batteryUpdate(delta);
    currentMeterUpdate(delta);
    powerMeterUpdate(delta);
    lastBatteryUpdate = newBatteryUpdate;

    // LOG_E(SYSTEM, "VBAT ADC: %d", adcGetChannel(ADC_BATTERY));
    uint16_t voltage = getBatteryVoltage();
    uint32_t power = getPower();
    uint32_t mAh = getMAhDrawn();
    uint32_t time = millis() / 1000;
    uint8_t seconds = time % 60;
    time /= 60;
    uint8_t minutes = time % 60;
    time /= 60;
    uint8_t bufLen = sprintf(lineBuffer, "%d.%02dV %ld.%01ldW %ld%c",
                             voltage / 100, voltage % 100, power / 100, (power % 100) / 10, mAh, 0x07 // What char is 0x07 ?
    );

    // Fill with spaces
    for (uint8_t i = bufLen; i < sizeof(lineBuffer); i++)
        lineBuffer[i] = ' ';
    lineBuffer[sizeof(lineBuffer) - 1] = '\0';

    // Run time
    sprintf(&lineBuffer[sizeof(lineBuffer) - 10], "%02ld:%02d:%02d%c",
            time, minutes, seconds, 0x70);

    // Bottom line: X.XXV X.XW X
    max7456Write(1, MAX7456_LINES_NTSC - 1, lineBuffer, 0);

    // Clear line buffer
    memset(&lineBuffer, 0, sizeof(lineBuffer));

    bool locked = animationLocked && (animationNumber == currentAnimation);
    if (animationNumber > BOOT_ANIMATION)
    {
        bufLen = sprintf(lineBuffer, "CUSTOM ANIMATION %d%s",
                         animationNumber - BOOT_ANIMATION, locked ? " LOCKED" : "");
    }
    else
    {
        bufLen = sprintf(lineBuffer, "%s%s", ANIMATION_NAME[animationNumber], locked ? " LOCKED" : "");
    }

    for (uint8_t i = bufLen; i < sizeof(lineBuffer); i++)
        lineBuffer[i] = ' ';
    lineBuffer[sizeof(lineBuffer) - 1] = '\0';

    max7456Write(1, MAX7456_LINES_NTSC - 2, lineBuffer, 0);

    max7456Update();
#endif
}
