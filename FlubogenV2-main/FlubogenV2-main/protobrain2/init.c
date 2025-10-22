#include "main.h"
#include "init.h"

bool hardwareInit(void) {
	systemInit();
	__enable_irq();
	IOInitGlobal();
	busInit();
	ledInit(false);
	usbVcpInitHardware();
	timerInit();
	serialInit(true, SERIAL_PORT_NONE);
    logInit();
	ws2811LedStripInit();
	sdcardInsertionDetectInit();
	max7456Init(0);			// 0 - AUTO, 1 - PAL, 2 - NTSC
	max7456ClearScreen();
	max7456RefreshAll();
	sdcard_init();
	afatfs_init();
	initEEPROM();
	//ensureEEPROMContainsValidData();
	//readEEPROM();

	drv_adc_config_t adc_params;
	memset(&adc_params, 0, sizeof(adc_params));
	adc_params.adcFunctionChannel[ADC_RSSI] = RSSI_ADC_CHANNEL;		// Brightness photoresistor
	adc_params.adcFunctionChannel[ADC_BATTERY] = VBAT_ADC_CHANNEL;
	adc_params.adcFunctionChannel[ADC_CURRENT] = CURRENT_METER_ADC_CHANNEL;
	adcInit(&adc_params);

	// Initialize battery measurements
	batteryInit();
	batteryUpdate(0);
	currentMeterUpdate(0);
	powerMeterUpdate(0);

	afatfsError_e sdError = afatfs_getLastError();
	afatfsFilesystemState_e fatState = afatfs_getFilesystemState();

	LED0_ON;

	while ((fatState != AFATFS_FILESYSTEM_STATE_READY) && (sdError == AFATFS_ERROR_NONE)) {
		afatfs_poll();
		sdError = afatfs_getLastError();
		fatState = afatfs_getFilesystemState();
	}

	LED0_OFF;

	if (sdError == AFATFS_ERROR_NONE) {
		LOG_E(SYSTEM, "SD card initialized");
	} else {
		LOG_E(SYSTEM, "SD card failed to initialize: %d", sdError);
		return false;
	}

	serial2 = (serialPort_t *)uartOpen(USART2, serialRxCallback, NULL, baudRates[BAUD_115200], MODE_RXTX, 0);

	return true;
}
