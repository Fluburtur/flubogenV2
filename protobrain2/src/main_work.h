/* Defines work items specific to the main module. */

#ifndef _MAIN_WORK_H_
#define _MAIN_WORK_H_

#include "work_queue.h"

typedef enum
{
    MAIN_WORK_CMD_READ_ADC_SENSORS,
    MAIN_WORK_CMD_UPDATE_OSD,
} main_work_item_command_t;

static_assert(sizeof(main_work_item_command_t) <= sizeof(work_command_t), "too big");

#endif /* _MAIN_WORK_H_ */
