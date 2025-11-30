/**
 * A global work queue.
 *
 * Modules will request work be done by adding items to the queue.
 * The main loop will consume work items and perform the work in order.
 */

#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_

typedef enum
{
    WORK_ITEM_ANIMATE_FACE_FRAME,
    WORK_ITEM_READ_ADC_SENSORS,
} work_item_t;

/**
 * Initialise the work queue.
 * */
void work_queue_init(void);

/**
 * Add a work item.
 *
 * Safe to call from interrupt handlers.
 *
 * @param[in] item Work item
 */
void work_queue_add(work_item_t item);

/**
 * Remove a work item.
 *
 * Blocks until work is available.
 */
work_item_t work_queue_remove_blocking(void);

#endif /* _WORK_QUEUE_H_ */
