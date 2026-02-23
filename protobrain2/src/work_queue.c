/**
 * The work queue has a fixed capacity, and we may assert if we try to add work to a full queue.
 * The queue capacity has been chosen so that it's large enough to deal with bursts of work,
 * but doesn't consume too much memory.
 */

#include <pico/util/queue.h>

#include "work_queue.h"

#define WORK_QUEUE_CAPACITY 10

static queue_t queue;

void work_queue_init(void)
{
    queue_init(&queue, sizeof(work_item_t), WORK_QUEUE_CAPACITY);
}

void work_queue_add(work_item_t item)
{
    bool added = queue_try_add(&queue, &item);
    hard_assert(added);
}

bool work_queue_try_add(work_item_t item)
{
    bool added = queue_try_add(&queue, &item);
    return added;
}

work_item_t work_queue_remove_blocking(void)
{
    work_item_t item;
    queue_remove_blocking(&queue, (void *)&item);
    return item;
}
