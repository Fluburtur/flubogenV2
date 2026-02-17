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

typedef enum
{
    /* Playing the boot animation. Not interruptible. */
    STATE_BOOTING,
    /* Playing the idle animation or a random animation.
     * Random animations are allowed. Animations can be interrupted by the remote. */
    STATE_IDLE_OR_RANDOM,
    /* Playing a specific animation requested by the remote. Go idle when it ends. */
    STATE_REMOTE_PLAY_ONCE,
    /* Playing a specific animation requested by the remote. Repeat when it ends. */
    STATE_REMOTE_PLAY_REPEAT,
    /* Playing the idle animation only. Other animation commands are ignored. */
    STATE_LOCKED_IDLE,
} animation_state_t;

/***********************
 * Variables
 ***********************/

static animation_state_t state;

/** Ticks at the animation framerate. Drives LED updates. */
static repeating_timer_t face_animation_timer;
static uint8_t repeating_animation_number;

/** We use this as a non-repeating timer. */
static alarm_id_t random_animation_timer;
static atomic_bool is_random_animation_timer_running;
static bool want_random_animation;

/***********************
 * Function prototypes
 ***********************/

static void work_booting(animation_work_item_command_t cmd, uint8_t param);
static void work_idle_or_random(animation_work_item_command_t cmd, uint8_t param);
static void work_remote_play_once(animation_work_item_command_t cmd, uint8_t param);
static void work_remote_play_repeat(animation_work_item_command_t cmd, uint8_t param);
static void work_locked_idle(animation_work_item_command_t cmd, uint8_t param);

static void start_frame_timer(uint16_t animation_period_ms);
static inline void stop_frame_timer(void);
static bool face_animation_callback(repeating_timer_t *timer);

static void start_random_animation_timer(void);
static void stop_random_animation_timer(void);
static int64_t random_animation_callback(alarm_id_t id, void *user_data);

/***********************
 * Public functions
 ***********************/

void animation_manager_init(void)
{
    state = STATE_BOOTING;
    is_random_animation_timer_running = false;

    hard_assert(animationInit());

    uint16_t animation_period_ms = startAnimation(BOOT_ANIMATION);
    start_frame_timer(animation_period_ms);
}

void animation_manager_handle_work(work_item_t work)
{
    hard_assert(work.destination == WORK_MODULE_ANIMATION);
    animation_work_item_command_t cmd = (animation_work_item_command_t)work.command;

    switch (state)
    {
    case STATE_BOOTING:
        work_booting(cmd, work.data);
        break;

    case STATE_IDLE_OR_RANDOM:
        work_idle_or_random(cmd, work.data);
        break;

    case STATE_REMOTE_PLAY_ONCE:
        work_remote_play_once(cmd, work.data);
        break;

    case STATE_REMOTE_PLAY_REPEAT:
        work_remote_play_repeat(cmd, work.data);
        break;

    case STATE_LOCKED_IDLE:
        work_locked_idle(cmd, work.data);
        break;
    }
}

bool animation_manager_is_locked(void)
{
    return state == STATE_LOCKED_IDLE;
}

/***********************
 * Private functions
 ***********************/

static void work_booting(animation_work_item_command_t cmd, uint8_t param)
{
    switch (cmd)
    {
    case ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME:
    {
        bool finished = updateAnimation();
        if (finished)
        {
            stop_frame_timer();

            /* a.k.a. the idle animation. */
            uint16_t animation_period_ms = startAnimation(DEFAULT_ANIMATION);
            start_frame_timer(animation_period_ms);

            /* We always start the random animation timer at the end of the boot animation. */
            start_random_animation_timer();

            state = STATE_IDLE_OR_RANDOM;
        }
    }
    break;

    case ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_ONCE:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_REPEAT:
    case ANIMATION_WORK_CMD_REMOTE_END_ANIMATION:
    case ANIMATION_WORK_CMD_REMOTE_TOGGLE_LOCK:
        /* All other requests are ignored. The boot animation is not interruptible. */
        (void)param;
        break;
    }
}

static void work_idle_or_random(animation_work_item_command_t cmd, uint8_t param)
{
    switch (cmd)
    {
    case ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME:
    {
        bool finished = updateAnimation();
        if (finished)
        {
            stop_frame_timer();

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

            if (!animationNumberIsValid(next_animation))
            {
                next_animation = DEFAULT_ANIMATION;
            }

            uint16_t animation_period_ms = startAnimation(next_animation);
            start_frame_timer(animation_period_ms);

            /* We start the random animation timer if it's not already running and we didn't
             * just start a random animation. In practice this means we start the timer at the
             * end of each random animation.
             * We do it like this so the time between random animations is correct if any of
             * the animations are long (which they are). */
            if (!started_random && !is_random_animation_timer_running)
            {
                start_random_animation_timer();
            }
        }
    }
    break;

    case ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION:
    {
        want_random_animation = true;
    }
    break;

    case ANIMATION_WORK_CMD_REMOTE_PLAY_ONCE:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_REPEAT:
    {
        /* The idle/random animation can be interrupted by an explicit request from the remote. */

        /* The remote should send command animation numbers 1-250 (but beware it could send an
         * invalid value).
         * These have to map to "real" animation numbers, where command animation 1 is actually
         * number 6 (first after the boot animation). */
        uint8_t command_animation_num = param;
        if ((command_animation_num == 0) || (command_animation_num > 250))
        {
            return;
        }
        uint8_t real_animation_num = BOOT_ANIMATION + command_animation_num;

        if (animationNumberIsValid(real_animation_num))
        {
            stop_random_animation_timer();
            want_random_animation = false;

            stop_frame_timer();
            uint16_t animation_period_ms = startAnimation(real_animation_num);
            start_frame_timer(animation_period_ms);

            if (cmd == ANIMATION_WORK_CMD_REMOTE_PLAY_ONCE)
            {
                state = STATE_REMOTE_PLAY_ONCE;
            }
            else
            {
                repeating_animation_number = real_animation_num;
                state = STATE_REMOTE_PLAY_REPEAT;
            }
        }
    }
    break;

    case ANIMATION_WORK_CMD_REMOTE_TOGGLE_LOCK:
    {
        /* "If the brain is unlocked and playing a random animation then it stops the random
         * animation and enters the locked state.
         * If the brain is unlocked and idle then it enters the locked state."
         *
         * We can deal with both of these by always stopping the current animation then starting
         * the idle animation. If we were already playing the idle animation it doesn't really
         * matter. */

        stop_random_animation_timer();
        want_random_animation = false;

        stop_frame_timer();
        uint16_t animation_period_ms = startAnimation(DEFAULT_ANIMATION);
        start_frame_timer(animation_period_ms);

        state = STATE_LOCKED_IDLE;
    }
    break;

    case ANIMATION_WORK_CMD_REMOTE_END_ANIMATION:
        /* Only has an effect in play-repeat mode. */
        break;
    }
}

static void work_remote_play_once(animation_work_item_command_t cmd, uint8_t param)
{
    switch (cmd)
    {
    case ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME:
    {
        bool finished = updateAnimation();
        if (finished)
        {
            stop_frame_timer();

            /* "When a remote-triggered animation finishes playing, go back to the idle
             * animation". */
            uint16_t animation_period_ms = startAnimation(DEFAULT_ANIMATION);
            start_frame_timer(animation_period_ms);

            /* We always start the random animation timer at the end of the remote-requested
             * animation. */
            start_random_animation_timer();

            state = STATE_IDLE_OR_RANDOM;
        }
    }
    break;

    case ANIMATION_WORK_CMD_REMOTE_TOGGLE_LOCK:
        /* "If the brain is unlocked but an animation is still playing from a previous command then
         * the toggle command is ignored." */
        break;

    case ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_ONCE:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_REPEAT:
    case ANIMATION_WORK_CMD_REMOTE_END_ANIMATION:
        /* All other requests are ignored. The animation is not interruptible. */
        (void)param;
        break;
    }
}

static void work_remote_play_repeat(animation_work_item_command_t cmd, uint8_t param)
{
    switch (cmd)
    {
    case ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME:
    {
        bool finished = updateAnimation();
        if (finished)
        {
            stop_frame_timer();

            /* Repeat the same animation. */
            uint16_t animation_period_ms = startAnimation(repeating_animation_number);
            start_frame_timer(animation_period_ms);
        }
    }
    break;

    case ANIMATION_WORK_CMD_REMOTE_END_ANIMATION:
    {
        /* We'll go idle when this animation ends. */
        state = STATE_REMOTE_PLAY_ONCE;
    }
    break;

    case ANIMATION_WORK_CMD_REMOTE_TOGGLE_LOCK:
        /* "If the brain is unlocked but an animation is still playing from a previous command then
         * the toggle command is ignored." */
        break;

    case ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_ONCE:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_REPEAT:
        /* All other requests are ignored. The animation is not interruptible. */
        (void)param;
        break;
    }
}

static void work_locked_idle(animation_work_item_command_t cmd, uint8_t param)
{
    switch (cmd)
    {
    case ANIMATION_WORK_CMD_ANIMATE_FACE_FRAME:
    {
        bool finished = updateAnimation();
        if (finished)
        {
            stop_frame_timer();

            /* Repeat the idle animation. */
            uint16_t animation_period_ms = startAnimation(DEFAULT_ANIMATION);
            start_frame_timer(animation_period_ms);
        }
    }
    break;

    case ANIMATION_WORK_CMD_REMOTE_TOGGLE_LOCK:
    {
        /* "If the brain is locked then it unlocks." */
        state = STATE_IDLE_OR_RANDOM;
    }
    break;

    case ANIMATION_WORK_CMD_REQUEST_RANDOM_ANIMATION:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_ONCE:
    case ANIMATION_WORK_CMD_REMOTE_PLAY_REPEAT:
    case ANIMATION_WORK_CMD_REMOTE_END_ANIMATION:
        /* All other requests are ignored. */
        (void)param;
        break;
    }
}

/* Must not be called if the timer is already running. */
static void start_frame_timer(uint16_t animation_period_ms)
{
    hard_assert(animation_period_ms != 0);
    hard_assert(
        add_repeating_timer_ms(
            animation_period_ms, face_animation_callback, NULL, &face_animation_timer));
}

/* Generally safe to call if the timer is already stopped, but try to avoid it. */
static inline void stop_frame_timer(void)
{
    cancel_repeating_timer(&face_animation_timer);
}

/* When the frame timer expires. Cause the next frame of animation to be displayed. */
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

/* Must not be called if the timer is already running. */
static void start_random_animation_timer(void)
{
    hard_assert(!is_random_animation_timer_running);
    random_animation_timer = add_alarm_in_ms(
        RANDOM_ANIMATION_PERIOD_MS, random_animation_callback, NULL, true);
    hard_assert(random_animation_timer > 0);
    is_random_animation_timer_running = true;
}

/* Safe to call if the timer is already stopped. */
static void stop_random_animation_timer(void)
{
    if (is_random_animation_timer_running)
    {
        is_random_animation_timer_running = false;
        cancel_alarm(random_animation_timer);
    }
}

/* When the random animation timer expires. Cause a random animation to be played when the current
 * animation finishes, if that's allowed. */
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
