#include <stdbool.h>
#include <stdint.h>

#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <pico/assert.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include "remote_msg.h"
#include "remote_work.h"
#include "work_queue.h"

/***********************
 * Defines and types
 ***********************/

/* Brain to remote. */

#define COMMAND_RESET 0xE2
#define COMMAND_START 0xE3

/* Remote to brain. */

#define COMMAND_HELLO 0xE1
#define COMMAND_PLAY_ANIMATION_ONCE 0x01
#define COMMAND_PLAY_ANIMATION_REPEAT 0x02
#define COMMAND_END_ANIMATION 0x03
#define COMMAND_TOGGLE_LOCK 0x04

/***********************
 * Function prototypes
 ***********************/

static void chars_available_irq_handler(void *param);

/***********************
 * Public functions
 ***********************/

void remote_msg_init(void)
{
    stdio_set_chars_available_callback(chars_available_irq_handler, NULL);
}

void remote_msg_send_reset(void)
{
    stdio_putchar_raw(COMMAND_RESET);
}

void remote_msg_send_start(void)
{
    stdio_putchar_raw(COMMAND_START);
}

/***********************
 * Private functions
 ***********************/

static void chars_available_irq_handler(void *param)
{
    /* The interrupt is triggered when some unknown number of characters are available on stdio.
     * Assuming they're all sent in one go via USB, they should appear "atomically". So we can
     * just read without blocking until all characters are consumed. */

    (void)param;

#define REQUIRE_BYTE(dst)                         \
    do                                            \
    {                                             \
        dst = 0;                                  \
        int result = stdio_getchar_timeout_us(0); \
        if (result == PICO_ERROR_TIMEOUT)         \
        {                                         \
            input_exhausted = true;               \
            break;                                \
        }                                         \
        dst = (uint8_t)(result & 0xFF);           \
    } while (false)

    bool input_exhausted = false;
    while (true)
    {
        work_item_t work_item;
        bool valid = false;

        uint8_t cmd;
        REQUIRE_BYTE(cmd);
        if (input_exhausted)
        {
            break;
        }

        switch (cmd)
        {
        case COMMAND_HELLO:
        {
            uint8_t protocol_version;
            REQUIRE_BYTE(protocol_version);

            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_HELLO;
            work_item.data = protocol_version;
            valid = true;
            break;
        }

        case COMMAND_PLAY_ANIMATION_ONCE:
        {
            uint8_t animation_number;
            REQUIRE_BYTE(animation_number);

            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_PLAY_ANIMATION_ONCE;
            work_item.data = animation_number;
            valid = true;
            break;
        }

        case COMMAND_PLAY_ANIMATION_REPEAT:
        {
            uint8_t animation_number;
            REQUIRE_BYTE(animation_number);

            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_PLAY_ANIMATION_REPEAT;
            work_item.data = animation_number;
            valid = true;
            break;
        }

        case COMMAND_END_ANIMATION:
        {
            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_END_ANIMATION;
            valid = true;
            break;
        }

        case COMMAND_TOGGLE_LOCK:
        {
            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_TOGGLE_LOCK;
            valid = true;
            break;
        }

        default:
            /* An unknown/invalid command. */
            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_INVALID;
            work_item.data = cmd;
            break;
        }

        if (valid)
        {
            /* Non-critical. The user can try again. */
            work_queue_try_add(work_item);
        }
        else
        {
            /* An unknown/invalid command, or a valid command was missing its parameter. */
            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_INVALID;
            work_item.data = cmd;
            /* Non-critical. If it doesn't fix itself we'll do this again next time. */
            work_queue_try_add(work_item);
        }
    }
}
