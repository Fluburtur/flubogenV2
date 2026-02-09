#pragma once

enum msg_from_brain_t {
    MSG_FROM_BRAIN_NONE,
    MSG_FROM_BRAIN_RESET,
    MSG_FROM_BRAIN_START,
};

msg_from_brain_t try_receive_message();

void send_message_hello();
void send_message_lock();
void send_message_play_animation_once(uint8_t animation_number);
void send_message_play_animation_repeat(uint8_t animation_number);
void send_message_end_animation();
