#ifndef _SD_CONFIG_H_
#define _SD_CONFIG_H_

/**
 * Initialise the SD card driver and mount the file system.
 *
 * Use `sd_config_is_card_ok()` to check if it actually succeeded.
 */
void sd_config_init(void);

/**
 * @return Is the SD card ok?
 */
bool sd_config_is_card_ok(void);

#endif /* #ifndef _SD_CONFIG_H_ */
