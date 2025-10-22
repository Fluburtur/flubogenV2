#include "gpio.h"

// Pin		Net			    Direction	Function	Fx      Description
// GPIO0	REMOTE_TX		Output      UART0 TX	F2      Serial out to Nano
// GPIO1	REMOTE_RX		Input       UART0 RX	F2      Serial in from Nano
// GPIO2	LED0			Output      			F5      Onboard LED, active high
// GPIO3	LED1			Output      			F5      Onboard LED, active high
// GPIO4	BOOP_TX			Output      		    F5      IR out
// GPIO5	BOOP_RX			Input       		    F5      IR in
// GPIO6	CLK				Output          		F5      SD card clock
// GPIO7	CMD				Output          		F5      SD card control / DI
// GPIO8	DAT0			SDIO		            F5      SD card data / DO
// GPIO9	DAT1			SDIO		            F5      SD card data / x
// GPIO10	DAT2			SDIO		            F5      SD card data / x
// GPIO11	DAT3			SDIO		            F5      SD card data / CS
// GPIO12	FACE_LED		Output      PWM6 A		F4      Face WS2812 LED chain
// GPIO13	CHEEK_LED		Output      PWM6 B		F4      Cheeks WS2812 LED chain
// GPIO14	BODY_LED0		Output      PWM7 A		F4      WS2812 LED chain
// GPIO15	BODY_LED1		Output      PWM7 B		F4      WS2812 LED chain
// GPIO16	OSD_CIPO		Input       SPI0 RX		F1      OSD chip data in
// GPIO17	OSD_nCS			Output      SPI0 CSn	F1      OSD chip select
// GPIO18	OSD_SCK			Output      SPI0 SCK	F1      OSD chip clock
// GPIO19	OSD_COPI		Output      SPI0 TX		F1      OSD chip data out
// GPIO26	BATT_VOLTAGE	Input       ADC0		F5      VBatt/11
// GPIO27	BATT_CURRENT	Input       ADC1		F5      25mOhm shunt
// GPIO28	BRIGHT			Input       ADC2		F5      Brightness sense

iodef_t iodefs[23] = {
	{0, 	GPIO_OUT,	GPIO_FUNC_UART, STATE_LOW},		// REMOTE_TX
	{1, 	GPIO_IN,	GPIO_FUNC_UART, PULL_DOWN},	    // REMOTE_RX
	{2, 	GPIO_OUT,	GPIO_FUNC_SIO, 	STATE_LOW},		// LED0
	{3, 	GPIO_OUT,	GPIO_FUNC_SIO, 	STATE_LOW},		// LED1
	{4, 	GPIO_OUT,	GPIO_FUNC_SIO, 	STATE_LOW},		// BOOP_TX
	{5, 	GPIO_IN,	GPIO_FUNC_SIO, 	STATE_LOW},		// BOOP_RX
	{6, 	GPIO_OUT,	GPIO_FUNC_SIO, 	STATE_LOW},		// CLK
	{7, 	GPIO_OUT,	GPIO_FUNC_SIO, 	STATE_LOW},		// CMD
	{8, 	GPIO_IN,	GPIO_FUNC_SIO, 	STATE_LOW},		// DAT0
	{9, 	GPIO_IN,	GPIO_FUNC_SIO, 	STATE_LOW},	    // DAT1
	
	{10, 	GPIO_IN,	GPIO_FUNC_SIO, 	STATE_LOW},	    // DAT2
	{11, 	GPIO_IN,	GPIO_FUNC_SIO, 	STATE_LOW},	    // DAT3
	{12, 	GPIO_OUT,	GPIO_FUNC_PWM,	STATE_LOW},		// FACE_LED
	{13, 	GPIO_OUT,	GPIO_FUNC_PWM,	STATE_LOW},		// CHEEK_LED
	{14, 	GPIO_OUT,	GPIO_FUNC_PWM,	STATE_LOW},		// BODY_LED0
	{15, 	GPIO_OUT,	GPIO_FUNC_PWM,	STATE_LOW},		// BODY_LED1
	{16,    GPIO_IN,	GPIO_FUNC_SPI, 	STATE_LOW},		// OSD_CIPO
	{17,    GPIO_OUT,	GPIO_FUNC_SPI, 	STATE_HIGH},	// OSD_nCS
	{18, 	GPIO_OUT,	GPIO_FUNC_SPI, 	STATE_LOW},		// OSD_SCK
	{19, 	GPIO_OUT,	GPIO_FUNC_SPI, 	STATE_LOW},		// OSD_COPI
	
	{26, 	GPIO_IN,	GPIO_FUNC_SIO, 	STATE_LOW},		// BATT_VOLTAGE
	{27, 	GPIO_IN,	GPIO_FUNC_SIO, 	STATE_LOW},		// BATT_CURRENT
	{28, 	GPIO_IN,	GPIO_FUNC_SIO, 	PULL_DOWN}		// BRIGHT
};
