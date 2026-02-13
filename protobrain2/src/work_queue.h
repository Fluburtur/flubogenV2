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
    WORK_MODULE_MAIN,
    WORK_MODULE_REMOTE,
    WORK_MODULE_ANIMATION,
} work_module_t;

typedef uint8_t work_command_t;
typedef uint8_t work_data_t;

typedef struct
{
    work_module_t destination;
    work_command_t command;
    work_data_t data;
} work_item_t;

/**
 * Initialise the work queue.
 * */
void work_queue_init(void);

/**
 * Add a work item.
 *
 * The item is treated as critical -- if there's no space in the work queue then we assert.
 * Safe to call from interrupt handlers.
 *
 * @param[in] item Work item
 */
void work_queue_add(work_item_t item);

/**
 * Try to add a work item.
 *
 * If there's no space in the work queue then the work item is thrown away.
 * Safe to call from interrupt handlers.
 *
 * @param[in] item Work item
 * @return true if the item was added to the queue
 */
bool work_queue_try_add(work_item_t item);

/**
 * Remove a work item.
 *
 * Blocks until work is available.
 */
work_item_t work_queue_remove_blocking(void);

#endif /* _WORK_QUEUE_H_ */
