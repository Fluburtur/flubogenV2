/* Sending and receiving messages via the serial. */

#include "config.h"

#include <Arduino.h>

#include "messages.h"

/***********************
 * Defines and types
 ***********************/

#define PROTOCOL_VERSION_1 1

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
 * Data
 ***********************/
/* None */

/***********************
 * Variables
 ***********************/
/* None */

/***********************
 * Functions
 ***********************/

msg_from_brain_t try_receive_message()
{
    /* Currently, all of the messages we can receive from the brain are a
     * single byte. */

    if (!Serial.available())
    {
        return MSG_FROM_BRAIN_NONE;
    }

    int cmd = Serial.read();
    switch (cmd)
    {
    case COMMAND_RESET:
        return MSG_FROM_BRAIN_RESET;

    case COMMAND_START:
        return MSG_FROM_BRAIN_START;

    /* An unrecognised command.
     * TODO: should we treat this as a reset, to try to get the brain and
     * remote back to normal operation? */
    default:
        return MSG_FROM_BRAIN_NONE;
    }
}

void send_message_hello()
{
#ifdef MESSAGES_DEBUGGING_HUMAN_READABLE
    Serial.println("MSG tx Hello");
#else
    uint8_t msg[2] = {COMMAND_HELLO, PROTOCOL_VERSION_1};
    Serial.write(msg, 2);
#endif
}

void send_message_toggle_lock()
{
#ifdef MESSAGES_DEBUGGING_HUMAN_READABLE
    Serial.println("MSG tx ToggleLock");
#else
    uint8_t msg = COMMAND_TOGGLE_LOCK;
    Serial.write(msg);
#endif
}

void send_message_play_animation_once(uint8_t animation_number)
{
#ifdef MESSAGES_DEBUGGING_HUMAN_READABLE
    Serial.print("MSG tx Play1 ");
    Serial.println(animation_number);
#else
    uint8_t msg[2] = {COMMAND_PLAY_ANIMATION_ONCE, animation_number};
    Serial.write(msg, 2);
#endif
}

void send_message_play_animation_repeat(uint8_t animation_number)
{
#ifdef MESSAGES_DEBUGGING_HUMAN_READABLE
    Serial.print("MSG tx PlayRep ");
    Serial.println(animation_number);
#else
    uint8_t msg[2] = {COMMAND_PLAY_ANIMATION_REPEAT, animation_number};
    Serial.write(msg, 2);
#endif
}

void send_message_end_animation()
{
#ifdef MESSAGES_DEBUGGING_HUMAN_READABLE
    Serial.println("MSG tx EndAnim");
#else
    uint8_t msg = COMMAND_END_ANIMATION;
    Serial.write(msg);
#endif
}
