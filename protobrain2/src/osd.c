#include "main.h"
#include "osd.h"

const uint8_t defaultCodePage[128] = {
		0,    0,    0,    0,    0,    0,    0,    0,
		0,    0,    0,    0,    0,    0,    0,    0,
		0,    0,    0,    0,    0,    0,    0,    0,
		0,    0,    0,    0,    0,    0,    0,    0,
	0x00,    0, 0x48,    0,    0,    0,    0, 0x46,
	0x3F, 0x40,    0,    0, 0x45, 0x49, 0x41, 0x47,
	0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x44, 0x43, 0x4A,    0, 0x4B, 0x42,
	0x4C, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
	0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21,
	0x22, 0x23, 0x24,    0,    0,    0,    0,    0,
		0, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
	0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
	0x3C, 0x3D, 0x3E,    0,    0,    0,    0,    0
};

uint16_t osd_buffer[480];	// 30 chars * 16 rows

void busWrite(uint8_t address, uint8_t data) {
	spi_get_hw(spi0)->dr = (uint32_t)((address << 8) | data);
	while(spi_is_busy(spi0)) {};

    while (spi_is_readable(spi0))
        (void)spi_get_hw(spi0)->dr;
}

uint16_t busRead(uint8_t address) {
	spi_get_hw(spi0)->dr = (address | 0x80) << 8;
	while(spi_is_busy(spi0)) {};

	return (uint16_t)spi_get_hw(spi0)->dr;
}

void osd_print(uint8_t x, uint8_t y, char const * str, uint8_t attr) {
	char c;
	uint16_t addr;

	addr = y * 30 + x;
	attr = (attr & 3) << 5;

	while((c = *str++)) {
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
void init_osd() {
	//spi_write_blocking(spi0, )

    // Wait for POR done
    //while(busRead(MAX7456_STAT) & 0x40);

    // Force software reset
    busWrite(MAX7456_VM0, 0x42);

    // Set screen buffer to all blanks
    for (uint32_t c = 0; c < 480; c++) {
        osd_buffer[c] = 0;
    }

    // Wait for software reset done
    while(busRead(MAX7456_VM0) & 0x02);

    // busWrite(MAX7456_VM0, 0b00100000);	// NTSC external sync, output off
	busWrite(MAX7456_VM0, 0b00110000); // 0b01001000 PAL auto sync, OSD on ---- orginal: 0x48
    // Set all rows to same black/white level
    for (uint32_t c = 0; c < 16; c++) {
        busWrite(MAX7456_RB0 + c, 0b00000001);	// 0% black, 100% white
		// busWrite(MAX7456_RB0 + c, 0b00001010);	// 0% black, 100% white
    }
	//busWrite(MAX7456_VM1, 0b11011100);	// Background 35% grey, 133ms blinking 50%
    busWrite(MAX7456_VM1, 0b01001000);	// Background 0% grey, 133ms blinking 50%

    busWrite(MAX7456_DMM, 0b01000000);	// Unique attributes mode, no auto-inc
	uint8_t factory_OSDBL = (uint8_t) busRead(MAX7456_OSDBL); // must read OSDBL first and rewrite factory values
    busWrite(MAX7456_OSDBL, factory_OSDBL & 0b11101111);
	//osd_print(12, 5, str_a, 3);

	// Display memory 0~511 = char, 512~1023 = attribute
	// Write to DMDI
	// Attribute byte: LBC, BLK, INV, 00000

    busWrite(MAX7456_VM0, 0b01001000);	// 0b01001000 PAL auto sync, OSD on ---- 0x48
}
// void init_osd() {
// 	//spi_write_blocking(spi0, )

//     // Wait for POR done
//     //while(busRead(MAX7456_STAT) & 0x40);

//     // Force software reset
//     busWrite(MAX7456_VM0, 0x42);

//     // Set screen buffer to all blanks
//     for (uint32_t c = 0; c < 480; c++) {
//         osd_buffer[c] = 0;
//     }

//     // Wait for software reset done
//     while(busRead(MAX7456_VM0) & 0x02);

//     // busWrite(MAX7456_VM0, 0b00100000);	// NTSC external sync, output off
// 	busWrite(MAX7456_VM0, 0b01101000); // 0b01001000 PAL auto sync, OSD on ---- orginal: 0x48
//     // Set all rows to same black/white level
//     for (uint32_t c = 0; c < 16; c++) {
//         busWrite(MAX7456_RB0 + c, 0b00000001);	// 0% black, 100% white
// 		// busWrite(MAX7456_RB0 + c, 0b00001010);	// 0% black, 100% white
//     }
// 	//busWrite(MAX7456_VM1, 0b11011100);	// Background 35% grey, 133ms blinking 50%
//     busWrite(MAX7456_VM1, 0b10001100);	// Background 0% grey, 133ms blinking 50%

//     busWrite(MAX7456_DMM, 0b01000000);	// Unique attributes mode, no auto-inc
// 	uint8_t factory_OSDBL = (uint8_t) busRead(MAX7456_OSDBL); // must read OSDBL first and rewrite factory values
//     busWrite(MAX7456_OSDBL, factory_OSDBL & 0b11101111);
// 	//osd_print(12, 5, str_a, 3);

// 	// Display memory 0~511 = char, 512~1023 = attribute
// 	// Write to DMDI
// 	// Attribute byte: LBC, BLK, INV, 00000

//     busWrite(MAX7456_VM0, 0b01101000);	// 0b01001000 PAL auto sync, OSD on ---- 0x48
// }

/*void updateOSD(uint8_t animationNumber) {
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

	//LOG_E(SYSTEM, "VBAT ADC: %d", adcGetChannel(ADC_BATTERY));
	uint16_t voltage = getBatteryVoltage();
	uint32_t power = getPower();
	uint32_t mAh = getMAhDrawn();
	uint32_t time = millis()/1000;
	uint8_t seconds = time % 60;
	time /= 60;
	uint8_t minutes = time % 60;
	time /= 60;
	uint8_t bufLen = sprintf(lineBuffer, "%d.%02dV %ld.%01ldW %ld%c",
			voltage/100, voltage%100, power/100, (power%100)/10, mAh, 0x07	// What char is 0x07 ?
	);

	// Fill with spaces
	for (uint8_t i = bufLen; i < sizeof(lineBuffer); i++)
		lineBuffer[i] = ' ';
	lineBuffer[sizeof(lineBuffer) - 1] = '\0';

	// Run time
	sprintf(&lineBuffer[sizeof(lineBuffer) - 10], "%02ld:%02d:%02d%c",
			time, minutes, seconds, 0x70
	);

	// Bottom line: X.XXV X.XW X
	max7456Write(1, MAX7456_LINES_NTSC - 1, lineBuffer, 0);

	// Clear line buffer
	memset(&lineBuffer, 0, sizeof(lineBuffer));

	bool locked = animationLocked && (animationNumber == currentAnimation);
	if (animationNumber > BOOT_ANIMATION) {
		bufLen = sprintf(lineBuffer, "CUSTOM ANIMATION %d%s",
				animationNumber - BOOT_ANIMATION, locked ? " LOCKED" : "");
	} else {
		bufLen = sprintf(lineBuffer, "%s%s", ANIMATION_NAME[animationNumber], locked ? " LOCKED" : "");
	}

	for (uint8_t i = bufLen; i < sizeof(lineBuffer); i++)
		lineBuffer[i] = ' ';
	lineBuffer[sizeof(lineBuffer) - 1] = '\0';

	max7456Write(1, MAX7456_LINES_NTSC - 2, lineBuffer, 0);
	
	max7456Update();
}*/
