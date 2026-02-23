/* Implements the remote protocol version 1 for an eight button remote.
 *
 * The meat is in connected_fsm.cpp -- go there to change the button mapping.
 */

#include "config.h"

#include "connected_fsm.h"
#include "messages.h"

/***********************
 * Defines and types
 ***********************/

/** The connection state between the remote and brain. */
enum connection_state_t
{
    STATE_UNCONNECTED,
    STATE_CONNECTED,
};

/***********************
 * Variables
 ***********************/

/** The time at which we last sent a Hello message. */
unsigned long last_hello;
/** The current connection state between the remote and brain. */
static connection_state_t connection_state;

/***********************
 * Functions
 ***********************/

void setup()
{
    last_hello = millis();

#ifdef MAIN_DEBUGGING
    connected_fsm_setup();
    connection_state = STATE_CONNECTED;
#else
    connection_state = STATE_UNCONNECTED;
#endif

    Serial.begin(115200);
#ifdef MAIN_DEBUGGING
    Serial.println("Main setup");
#endif
}

void reset()
{
#ifdef MAIN_DEBUGGING
    Serial.println("Main reset");
#endif

    Serial.end();
    setup();
}

void loop()
{
    /* Process any messages from the brain. They might change our connection
     * state. */

    msg_from_brain_t brain_msg = try_receive_message();
    switch (brain_msg)
    {
    case MSG_FROM_BRAIN_NONE:
    default:
        /* Continue normally. */
        break;

    case MSG_FROM_BRAIN_RESET:
        /* Reset all of our state. */
        reset();
        return;

    case MSG_FROM_BRAIN_START:
        /* We should only get this in response to our "hello" message.
         * We can now start sending commands. */
        if (connection_state == STATE_UNCONNECTED)
        {
            connection_state = STATE_CONNECTED;
            connected_fsm_setup();
        }
        break;
    }

    /* Do whatever is appropriate for our current state. */

    switch (connection_state)
    {
    case STATE_UNCONNECTED:
        work_unconnected();
        break;

    case STATE_CONNECTED:
        connected_fsm_do_work();
        break;
    }
}

static void work_unconnected()
{
    /* "On power up, the remote must send only the "hello" message once every
     * second until a reply is received." */

    unsigned long now = millis();
    if (now > (last_hello + HELLO_INTERVAL_MS))
    {
        send_message_hello();
        last_hello = now;
    }
}
