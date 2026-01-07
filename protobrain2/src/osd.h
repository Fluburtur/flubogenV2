#ifndef _OSD_H_
#define _OSD_H_

void osd_init(void);
void osd_update(
  uint16_t ADCAvgBattV, uint32_t msSinceBoot, const char *animation_name,
  const uint8_t *lastRemoteData, bool sd_card_ok);

#endif /* _OSD_H_ */
