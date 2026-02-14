/* Interacts with the remote control, dispatches its requests to the rest of the system. */

#ifndef _REMOTE_H_
#define _REMOTE_H_

#include "work_queue.h"

/**
 * Initialise the remote control module.
 */
void remote_init(void);

/**
 * Handle the remote module's work items.
 *
 * @param[in] work Work item
 */
void remote_handle_work(work_item_t work);

#endif /* _REMOTE_H_ */
