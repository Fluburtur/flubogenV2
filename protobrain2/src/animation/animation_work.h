/* Defines work items specific to the animation manager module. */

#ifndef _ANIMATION_WORK_H_
#define _ANIMATION_WORK_H_

#include "work_queue.h"

typedef enum
{
    ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME,
    ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION,
} animation_work_item_command_t;

static_assert(sizeof(animation_work_item_command_t) <= sizeof(work_command_t), "too big");

#endif /* _ANIMATION_WORK_H_ */
