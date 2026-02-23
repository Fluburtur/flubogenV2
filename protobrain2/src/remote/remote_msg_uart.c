#include <stdbool.h>
#include <stdint.h>

#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/uart.h>
#include <pico/assert.h>
#include <pico/time.h>

#include "remote_msg.h"
#include "remote_work.h"
#include "work_queue.h"

/***********************
 * Defines and types
 ***********************/

#if 1
/* Normally the remote is on GPIO 0 and 1... */
#define REMOTE_UART_INSTANCE uart0
#define GPIO_PIN_TX 0
#define GPIO_PIN_RX 1
#else
/* But we move it to GPIO 4 and 5 if we somehow managed to break the normal pins... */
#define REMOTE_UART_INSTANCE uart1
#define GPIO_PIN_TX 4
#define GPIO_PIN_RX 5
#endif

#define UART_BAUD_RATE 115200

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

static void uart_rx_irq_handler(void);

/***********************
 * Public functions
 ***********************/

void remote_msg_init(void)
{
    gpio_set_function(GPIO_PIN_TX, UART_FUNCSEL_NUM(REMOTE_UART_INSTANCE, GPIO_PIN_TX));
    gpio_set_function(GPIO_PIN_RX, UART_FUNCSEL_NUM(REMOTE_UART_INSTANCE, GPIO_PIN_RX));
    uart_init(REMOTE_UART_INSTANCE, UART_BAUD_RATE);

    irq_set_exclusive_handler(UART_IRQ_NUM(REMOTE_UART_INSTANCE), uart_rx_irq_handler);
    irq_set_enabled(UART_IRQ_NUM(REMOTE_UART_INSTANCE), true);
    // uart_set_irqs_enabled(REMOTE_UART_INSTANCE, true, false);
    /* Enable the read timeout interrupt only. */
    uart_get_hw(REMOTE_UART_INSTANCE)->imsc = 1 << UART_UARTIMSC_RTIM_LSB;
}

void remote_msg_send_reset(void)
{
    uart_putc_raw(REMOTE_UART_INSTANCE, COMMAND_RESET);
}

void remote_msg_send_start(void)
{
    uart_putc_raw(REMOTE_UART_INSTANCE, COMMAND_START);
}

/***********************
 * Private functions
 ***********************/

static void uart_rx_irq_handler(void)
{
    /* The interrupt is triggered only on RX timeout.
     * I.e. there is at least 1 byte and the remote hasn't sent anything for 32 bit periods.
     * "The receive timeout interrupt is cleared either when the FIFO becomes empty through reading
     * all the data, or when a 1 is written to the corresponding bit of the Interrupt Clear
     * Register"
     *
     * We use just this interrupt because we expect the remote to send 1 or 2 bytes, wait for a
     * human-scale event (tens of milliseconds), send 1 or 2 bytes...
     * At 115200 baud, 32 bit periods is 8.7 usec * 32 = 278 usec. So this is a nice way to be
     * pretty sure we'll have all the data that the remote wanted to send sitting in the FIFO. */

#define REQUIRE_BYTE(dst)                                           \
    do                                                              \
    {                                                               \
        dst = 0;                                                    \
        if (!uart_is_readable(REMOTE_UART_INSTANCE))                \
        {                                                           \
            break;                                                  \
        }                                                           \
        uint32_t word = uart_get_hw(REMOTE_UART_INSTANCE)->dr;      \
        /* TODO: don't ignore the error bits in the FIFO values. */ \
        dst = (uint8_t)(word & 0xFF);                               \
    } while (false)

    irq_set_enabled(UART_IRQ_NUM(REMOTE_UART_INSTANCE), false);

    while (uart_is_readable(REMOTE_UART_INSTANCE))
    {
        work_item_t work_item;
        bool valid = false;

        uint8_t cmd;
        REQUIRE_BYTE(cmd);

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
        }
        break;

        case COMMAND_PLAY_ANIMATION_ONCE:
        {
            uint8_t animation_number;
            REQUIRE_BYTE(animation_number);

            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_PLAY_ANIMATION_ONCE;
            work_item.data = animation_number;
            valid = true;
        }
        break;

        case COMMAND_PLAY_ANIMATION_REPEAT:
        {
            uint8_t animation_number;
            REQUIRE_BYTE(animation_number);

            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_PLAY_ANIMATION_REPEAT;
            work_item.data = animation_number;
            valid = true;
        }
        break;

        case COMMAND_END_ANIMATION:
        {
            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_END_ANIMATION;
            valid = true;
        }
        break;

        case COMMAND_TOGGLE_LOCK:
        {
            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_TOGGLE_LOCK;
            valid = true;
        }
        break;

        default:
        {
            /* An unknown/invalid command. */
            work_item.destination = WORK_MODULE_REMOTE;
            work_item.command = REMOTE_WORK_CMD_RX_INVALID;
            work_item.data = cmd;
        }
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

    // Clear all interrupts.
    uart_get_hw(REMOTE_UART_INSTANCE)->icr = UART_UARTICR_BITS;
    irq_set_enabled(UART_IRQ_NUM(REMOTE_UART_INSTANCE), true);
}
