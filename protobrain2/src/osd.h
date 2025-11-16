#ifndef _OSD_H_
#define _OSD_H_

#define MAX7456_READ         0x80
#define MAX7456_VM0          0x00
#define MAX7456_VM1          0x01
#define MAX7456_HOS          0x02
#define MAX7456_VOS          0x03
#define MAX7456_DMM          0x04
#define MAX7456_DMAH         0x05
#define MAX7456_DMAL         0x06
#define MAX7456_DMDI         0x07
#define MAX7456_CMM          0x08
#define MAX7456_CMAH         0x09
#define MAX7456_CMAL         0x0a
#define MAX7456_CMDI         0x0b
#define MAX7456_OSDM         0x0c
#define MAX7456_RB0          0x10
#define MAX7456_RB1          0x11
#define MAX7456_RB2          0x12
#define MAX7456_RB3          0x13
#define MAX7456_RB4          0x14
#define MAX7456_RB5          0x15
#define MAX7456_RB6          0x16
#define MAX7456_RB7          0x17
#define MAX7456_RB8          0x18
#define MAX7456_RB9          0x19
#define MAX7456_RB10         0x1a
#define MAX7456_RB11         0x1b
#define MAX7456_RB12         0x1c
#define MAX7456_RB13         0x1d
#define MAX7456_RB14         0x1e
#define MAX7456_RB15         0x1f
#define MAX7456_OSDBL        0x6c
#define MAX7456_STAT         0xA0
#define MAX7456_CMDO         0xC0
#define MAX7456_PAL_ROWS     16

uint16_t busRead(uint8_t address);
void busWrite(uint8_t address, uint8_t data);
void init_osd();
void osd_print(uint8_t x, uint8_t y, char const * str, uint8_t attr);
//void updateOSD(uint8_t animationNumber);

#endif /* _OSD_H_ */
