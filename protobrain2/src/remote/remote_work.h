/* Defines work items specific to the remote module. */

#ifndef _REMOTE_WORK_H_
#define _REMOTE_WORK_H_

#include "work_queue.h"

typedef enum
{
    REMOTE_WORK_CMD_CONNECTION_TIMEOUT,
    REMOTE_WORK_CMD_RX_INVALID,
    REMOTE_WORK_CMD_RX_HELLO,
    REMOTE_WORK_CMD_RX_PLAY_ANIMATION_ONCE,
    REMOTE_WORK_CMD_RX_PLAY_ANIMATION_REPEAT,
    REMOTE_WORK_CMD_RX_END_ANIMATION,
    REMOTE_WORK_CMD_RX_TOGGLE_LOCK,
} remote_work_item_command_t;

static_assert(sizeof(remote_work_item_command_t) <= sizeof(work_command_t), "too big");

#endif /* _REMOTE_WORK_H_ */
