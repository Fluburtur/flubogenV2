#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pico/assert.h>
#include <pico/time.h>

#include "anim.h"
#include "animation_manager.h"
#include "animation_work.h"
#include "work_queue.h"

/***********************
 * Defines and types
 ***********************/

#define REPEATING_TIMER_CONTINUE true
#define ALARM_STOP 0
#define ALARM_RESCHEDULE_AFTER_MS(ms) (-(ms))

/** Play a random animation every 10 seconds. */
#define RANDOM_ANIMATION_PERIOD_MS (10 * 1000)

/***********************
 * Variables
 ***********************/

static uint16_t animation_period_ms;
static repeating_timer_t face_animation_timer;

/** We use this as a non-repeating timer. */
static alarm_id_t random_animation_timer;
static atomic_bool is_random_animation_timer_running;
static bool want_random_animation;

/***********************
 * Function prototypes
 ***********************/

static bool face_animation_callback(repeating_timer_t *timer);
static int64_t random_animation_callback(alarm_id_t id, void *user_data);

/***********************
 * Public functions
 ***********************/

void animation_manager_init(void)
{
    is_random_animation_timer_running = false;

    hard_assert(animationInit());

    animation_period_ms = startAnimation(BOOT_ANIMATION);
    hard_assert(animation_period_ms != 0);

    hard_assert(
        add_repeating_timer_ms(
            animation_period_ms, face_animation_callback, NULL, &face_animation_timer));
}

void animation_manager_handle_work(work_item_t work)
{
    hard_assert(work.destination == WORK_MODULE_ANIMATION);
    animation_work_item_command_t cmd = (animation_work_item_command_t)work.command;

    switch (cmd)
    {
    case ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME:
    {
        bool finished = updateAnimation();
        if (finished)
        {
            cancel_repeating_timer(&face_animation_timer);

            /* a.k.a. the idle animation. */
            uint8_t next_animation = DEFAULT_ANIMATION;
            bool started_random = false;

            if (want_random_animation)
            {
                started_random = true;
                want_random_animation = false;

                switch (rand() % 4)
                {
                case 0:
                    next_animation = RANDOM_ANIMATION_1;
                    break;
                case 1:
                    next_animation = RANDOM_ANIMATION_2;
                    break;
                case 2:
                    next_animation = RANDOM_ANIMATION_3;
                    break;
                default:
                    /* Intentionally have a chance to pick the default animation. */
                    next_animation = DEFAULT_ANIMATION;
                    break;
                }
            }

            animation_period_ms = startAnimation(next_animation);
            hard_assert(
                add_repeating_timer_ms(
                    animation_period_ms, face_animation_callback, NULL, &face_animation_timer));

            /* We start the random animation timer if it's not already running and we didn't
             * just start a random animation. In practice this means we start the timer at the
             * end of the boot animation, and then at the end of each random animation.
             * We do it like this so the time between random animations is correct if any of
             * the animations are long (which they are). */
            if (!started_random && !is_random_animation_timer_running)
            {
                random_animation_timer = add_alarm_in_ms(
                    RANDOM_ANIMATION_PERIOD_MS, random_animation_callback, NULL, true);
                hard_assert(random_animation_timer > 0);
                is_random_animation_timer_running = true;
            }
        }
    }
    break;

    case ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION:
    {
        want_random_animation = true;
        break;
    }
    }
}

/***********************
 * Private functions
 ***********************/

static bool face_animation_callback(repeating_timer_t *timer)
{
    (void)timer;
    work_item_t work = {
        .destination = WORK_MODULE_ANIMATION,
        .command = ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME,
    };
    work_queue_try_add(work);
    /* Assume we want to draw more frames. If the work queue is full this just results in a
     * temporarily lower framerate. */
    return REPEATING_TIMER_CONTINUE;
}

static int64_t random_animation_callback(alarm_id_t id, void *user_data)
{
    (void)id;
    (void)user_data;
    work_item_t work = {
        .destination = WORK_MODULE_ANIMATION,
        .command = ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION,
    };
    if (work_queue_try_add(work))
    {
        is_random_animation_timer_running = false;
        return ALARM_STOP;
    }
    else
    {
        /* If the work queue is full we'll try again later. */
        return ALARM_RESCHEDULE_AFTER_MS(RANDOM_ANIMATION_PERIOD_MS);
    }
}
