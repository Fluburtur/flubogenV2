#include <stdbool.h>
#include <stdint.h>

#include <pico/assert.h>
#include <pico/time.h>

#include "animation/animation_work.h"
#include "remote.h"
#include "remote_msg.h"
#include "remote_work.h"
#include "work_queue.h"

/***********************
 * Defines and types
 ***********************/

#define REPEATING_TIMER_CONTINUE true

/* "When the brain boots, it should wait 10 seconds for the remote to send a 'hello' message." */
#define PROTOCOL_CONNECT_TIMEOUT_MS (10 * 1000)

#define PROTOCOL_VERSION_1 1

typedef enum
{
    STATE_UNCONNECTED,
    STATE_CONNECTED,
} remote_state_t;

/***********************
 * Variables
 ***********************/

static remote_state_t state;

/* If the remote doesn't connect within a certain time, reset it. */
static repeating_timer_t connect_timeout_timer;

/***********************
 * Function prototypes
 ***********************/

static void start_connect_timeout(void);
static void cancel_connect_timeout(void);
static bool connect_timeout_callback(repeating_timer_t *timer);

static void work_unconnected(remote_work_item_command_t cmd, uint8_t data);
static void work_connected(remote_work_item_command_t cmd, uint8_t data);

/***********************
 * Public functions
 ***********************/

void remote_init(void)
{
    state = STATE_UNCONNECTED;
    start_connect_timeout();
    remote_msg_init();
}

void remote_handle_work(work_item_t work)
{
    hard_assert(work.destination == WORK_MODULE_REMOTE);
    remote_work_item_command_t cmd = (remote_work_item_command_t)work.command;

    switch (state)
    {
    case STATE_UNCONNECTED:
        work_unconnected(cmd, work.data);
        break;

    case STATE_CONNECTED:
        work_connected(cmd, work.data);
        break;
    }
}

/***********************
 * Private functions
 ***********************/

static void start_connect_timeout(void)
{
    hard_assert(
        add_repeating_timer_ms(
            PROTOCOL_CONNECT_TIMEOUT_MS,
            connect_timeout_callback,
            NULL,
            &connect_timeout_timer));
}

static void cancel_connect_timeout(void)
{
    cancel_repeating_timer(&connect_timeout_timer);
}

/* The remote hasn't connected in good time.
 * We want to send it a reset command to attempt to restore communication. */
static bool connect_timeout_callback(repeating_timer_t *timer)
{
    (void)timer;

    /* We're in interrupt context. Handle this in normal context. */
    work_item_t work = {
        .destination = WORK_MODULE_REMOTE,
        .command = REMOTE_WORK_CMD_CONNECTION_TIMEOUT,
    };
    work_queue_add(work);

    /* Keep trying periodically until the remote connects. */
    return REPEATING_TIMER_CONTINUE;
}

static void work_unconnected(remote_work_item_command_t cmd, uint8_t param)
{
    switch (cmd)
    {
    case REMOTE_WORK_CMD_CONNECTION_TIMEOUT:
    {
        /* "When the brain boots, it should wait 10 seconds for the remote to send a "hello"
         * message. If this does not happen... the brain should send a "reset" command to the
         * remote every 10 seconds" */
        remote_msg_send_reset();
    }
    break;

    case REMOTE_WORK_CMD_RX_HELLO:
    {
        if (param == PROTOCOL_VERSION_1)
        {
            /* The response to the "hello" message is the "start" message.*/
            cancel_connect_timeout();
            remote_msg_send_start();
            state = STATE_CONNECTED;
        }
        else
        {
            /* We don't support other protocol versions. We'll just ignore the message, and not
             * send a reply.
             * TODO: handle an unsupported remote protocol instead of just ignoring it? */
        }
    }
    break;

    case REMOTE_WORK_CMD_RX_INVALID:
    case REMOTE_WORK_CMD_RX_PLAY_ANIMATION_ONCE:
    case REMOTE_WORK_CMD_RX_PLAY_ANIMATION_REPEAT:
    case REMOTE_WORK_CMD_RX_END_ANIMATION:
    case REMOTE_WORK_CMD_RX_TOGGLE_LOCK:
    {
        /* If we receive anything else from the remote we're out of sync.
         * Restart the connection timer and send a reset command. */
        cancel_connect_timeout();
        start_connect_timeout();
        remote_msg_send_reset();
    }
    break;
    }
}

static void work_connected(remote_work_item_command_t cmd, uint8_t param)
{
    switch (cmd)
    {
    case REMOTE_WORK_CMD_CONNECTION_TIMEOUT:
    {
        /* Unlikely race condition. Cancel the conection timeout and stay connected. */
        cancel_connect_timeout();
    }
    break;

    case REMOTE_WORK_CMD_RX_HELLO:
    {
        /* "The brain should be prepared to receive a "hello" message at any time, in case the
         * remote crashes and reboots."*/

        if (param == PROTOCOL_VERSION_1)
        {
            /* The response to the "hello" message is the "start" message.*/
            remote_msg_send_start();
            state = STATE_CONNECTED;
        }
        else
        {
            /* We don't support other protocol versions.
             * It's weird that this has happened, since to get to the connected state the remote
             * must have previously sent the correct protocol version. Assume something bad has
             * happened, reset the remote, and go back to the unconncted state. */
            state = STATE_UNCONNECTED;
            start_connect_timeout();
            remote_msg_send_reset();
        }
    }
    break;

    case REMOTE_WORK_CMD_RX_INVALID:
    {
        /* Assume something bad has happened, reset the remote, and go back to the unconnected
         * state. */
        state = STATE_UNCONNECTED;
        start_connect_timeout();
        remote_msg_send_reset();
    }
    break;

    case REMOTE_WORK_CMD_RX_PLAY_ANIMATION_ONCE:
    {
        work_item_t work = {
            .destination = WORK_MODULE_ANIMATION,
            .command = ANIMATION_WORK_CMD_REMOTE_PLAY_ONCE,
            .data = param,
        };
        work_queue_try_add(work);
    }
    break;

    case REMOTE_WORK_CMD_RX_PLAY_ANIMATION_REPEAT:
    {
        work_item_t work = {
            .destination = WORK_MODULE_ANIMATION,
            .command = ANIMATION_WORK_CMD_REMOTE_PLAY_REPEAT,
            .data = param,
        };
        work_queue_try_add(work);
    }
    break;

    case REMOTE_WORK_CMD_RX_END_ANIMATION:
    {
        work_item_t work = {
            .destination = WORK_MODULE_ANIMATION,
            .command = ANIMATION_WORK_CMD_REMOTE_END_ANIMATION,
            .data = param,
        };
        work_queue_try_add(work);
    }
    break;

    case REMOTE_WORK_CMD_RX_TOGGLE_LOCK:
    {
        work_item_t work = {
            .destination = WORK_MODULE_ANIMATION,
            .command = ANIMATION_WORK_CMD_REMOTE_TOGGLE_LOCK,
        };
        work_queue_try_add(work);
    }
    break;
    }
}
