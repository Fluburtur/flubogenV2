#include "ButtonsBits.h"

/* If this changes then the whole implementation needs to be updated manually. */
#define NUM_BUTTONS 8

ButtonsBits::ButtonsBits() : button_bits(0) {}

void ButtonsBits::clear_all()
{
    button_bits = 0;
}

void ButtonsBits::clear(int button_idx)
{
    if (button_idx < 0)
        return;
    if (button_idx >= NUM_BUTTONS)
        return;

    uint8_t mask = ~(uint8_t)(1 << button_idx);
    button_bits &= mask;
}

void ButtonsBits::clear_raw(uint8_t mask)
{
    button_bits &= ~mask;
}

void ButtonsBits::set(int button_idx)
{
    if (button_idx < 0)
        return;
    if (button_idx >= NUM_BUTTONS)
        return;

    button_bits |= (uint8_t)(1 << button_idx);
}

void ButtonsBits::set_raw(uint8_t mask)
{
    button_bits |= mask;
}

uint8_t ButtonsBits::get_raw()
{
    return button_bits;
}

bool ButtonsBits::is_set(int button_idx)
{
    if (button_idx < 0)
        return false;
    if (button_idx >= NUM_BUTTONS)
        return false;

    return button_bits & (uint8_t)(1 << button_idx);
}

bool ButtonsBits::is_single_set(int button_idx)
{
    if (button_idx < 0)
        return false;
    if (button_idx >= NUM_BUTTONS)
        return false;

    return button_bits == (uint8_t)(1 << button_idx);
}

bool ButtonsBits::get_single_set(int &button_idx)
{
    for (uint8_t i = 0; i < NUM_BUTTONS; i++)
    {
        if (button_bits == (uint8_t)(1 << i))
        {
            button_idx = i;
            return true;
        }
    }

    return false;
}

bool ButtonsBits::is_any_set()
{
    return button_bits != 0;
}

bool ButtonsBits::is_all_clear()
{
    return button_bits == 0;
}
