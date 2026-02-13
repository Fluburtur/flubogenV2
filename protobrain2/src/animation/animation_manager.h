/* Manages animations. Play a particular one, idle, start random, etc. */

#ifndef _ANIMATION_MANAGER_H_
#define _ANIMATION_MANAGER_H_

#include "work_queue.h"

/**
 * Initialise the animation manager module.
 */
void animation_manager_init(void);

/**
 * Handle the animation manager module's work items.
 *
 * @param[in] work Work item
 */
void animation_manager_handle_work(work_item_t work);

#endif /* _ANIMATION_MANAGER_H_ */
