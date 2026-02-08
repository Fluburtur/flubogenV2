/////////////////////////////////////////////////////////////////
/*
  Butt2Mod.cpp - Arduino Library to simplify working with buttons.
  Based on the Button2 library by Lennart Hennigs
  https://github.com/LennartHennigs/Button2 at commit 9ca66f2bb91c8ca78f45eed376b027d9e9bf3501


  Copyright (C) 2017-2025 Lennart Hennigs.
  Released under the MIT license.
*/
/////////////////////////////////////////////////////////////////

#include "config.h"

#include "Butt2Mod.h"

#define DO_CALLBACK(cb_fn) \
    do                     \
    {                      \
        if (cb_fn != NULL) \
            cb_fn(*this);  \
    } while (false)

/////////////////////////////////////////////////////////////////
// initialize static counter for the IDs

int Butt2Mod::_nextID = 0;

/////////////////////////////////////////////////////////////////
//  default constructor

Butt2Mod::Butt2Mod()
{
    pin = BTN_UNDEFINED_PIN;
    _setDefaultID();
}

/////////////////////////////////////////////////////////////////
// constructor

Butt2Mod::Butt2Mod(
    uint8_t attachTo,
    uint8_t buttonMode /* = INPUT_PULLUP */,
    bool activeLow /* = true */)
{
    begin(attachTo, buttonMode, activeLow);
    _setDefaultID();
}

/////////////////////////////////////////////////////////////////

void Butt2Mod::begin(
    uint8_t attachTo,
    uint8_t buttonMode /* = INPUT_PULLUP */,
    bool activeLow /* = true */,
    InitCallbackFunction initCallback /* = NULL */)
{
    pin = attachTo;
    _activeLevel = activeLow ? LOW : HIGH;

    // Call initialization callback if provided (useful for I2C/SPI expanders, touch sensors, etc.)
    if (initCallback != NULL)
    {
        initCallback();
    }

    if (attachTo != BTN_VIRTUAL_PIN)
    {
        pinMode(attachTo, buttonMode);
    }

    curr_level = _getLevel();
    prev_level = curr_level;
}

/////////////////////////////////////////////////////////////////
// trivial stuff

void Butt2Mod::setDebounceDuration(unsigned int ms)
{
    debounce_duration_ms = ms;
}

void Butt2Mod::setDoubleClickDuration(unsigned int ms)
{
    doubleclick_duration_ms = ms;
}

unsigned int Butt2Mod::getDebounceDuration() const
{
    return debounce_duration_ms;
}

unsigned int Butt2Mod::getDoubleClickDuration() const
{
    return doubleclick_duration_ms;
}

uint8_t Butt2Mod::getPin() const
{
    return pin;
}

void Butt2Mod::setButtonLevelFunction(LevelCallbackFunction f)
{
    get_level_fn = f;
}

bool Butt2Mod::operator==(const Butt2Mod &rhs) const
{
    return (this == &rhs);
}

void Butt2Mod::setPressHandler(CallbackFunction f)
{
    press_cb = f;
}

void Butt2Mod::setSingleClickHandler(CallbackFunction f)
{
    single_click_cb = f;
}

void Butt2Mod::setSingleHoldHandler(CallbackFunction f)
{
    single_hold_cb = f;
}

void Butt2Mod::setDoubleClickHandler(CallbackFunction f)
{
    double_click_cb = f;
}

void Butt2Mod::setDoubleHoldHandler(CallbackFunction f)
{
    double_hold_cb = f;
}

void Butt2Mod::setHoldEndedHandler(CallbackFunction f)
{
    hold_ended_cb = f;
}

bool Butt2Mod::isPressed() const
{
    return (curr_level == _activeLevel);
}

bool Butt2Mod::isPressedRaw() const
{
    return (_getLevel() == _activeLevel);
}

int Butt2Mod::getID() const
{
    return id;
}

void Butt2Mod::setID(int newID)
{
    id = newID;
}

void Butt2Mod::_setDefaultID()
{
    id = _nextID;
    _nextID++;
}

/////////////////////////////////////////////////////////////////

void Butt2Mod::resetPressedState()
{
    state = b2m_state::idle;
    down1_instant_ms = 0;
    down2_instant_ms = 0;
    debouncing_end_instant_ms = 0;
}

/////////////////////////////////////////////////////////////////

void Butt2Mod::reset()
{
    pin = BTN_UNDEFINED_PIN;

    resetPressedState();

    get_level_fn = NULL;
    single_click_cb = NULL;
    single_hold_cb = NULL;
    double_click_cb = NULL;
    double_hold_cb = NULL;
    hold_ended_cb = NULL;
}

/////////////////////////////////////////////////////////////////

// IMPORTANT: This function must be called regularly for the button library to work.
// Recommended call frequency: Every 1-10ms for optimal responsiveness and timing accuracy.
//
// Timing accuracy considerations:
// - Debouncing depends on precise timing measurements
// - Multi-click detection relies on timeout windows
// - Hold detection requires continuous monitoring
//
// If loop() is not called frequently enough:
// - Debouncing may not work correctly (missed bounces)
// - Double click detection may fail (missed clicks)
// - Hold timing will be less accurate
//
// Example good practice:
//   void loop() {
//     button.loop();  // Call early and often
//     // ... other non-blocking code
//   }
//
// Avoid:
//   - Long delay() calls between loop() invocations
//   - Blocking operations that prevent regular calling
//   - Calling less frequently than ~10ms
void Butt2Mod::loop()
{
    if (pin == BTN_UNDEFINED_PIN)
        return;

    unsigned long now = millis();
    if (now < debouncing_end_instant_ms)
    {
#ifdef BUTT2MOD_DEBUGGING
        if (id == 0)
        {
            Serial.println("debouncing");
        }
#endif
        return;
    }

    prev_level = curr_level;
    curr_level = _getLevel();

    if (curr_level == _activeLevel)
    {
        _whenDown(now);
    }
    else
    {
        _whenUp(now);
    }
}

/////////////////////////////////////////////////////////////////

void Butt2Mod::_whenDown(unsigned long now)
{
    // If we're going from unpressed to pressed.
    if (prev_level != _activeLevel)
    {
        // Debouncing strategy: ignore level changes for the debounce duration.
        // If a genuine button release happens during the debounce window then
        // it will just be deferred until the end of the window.
        debouncing_end_instant_ms = now + debounce_duration_ms;

        if (state == b2m_state::idle)
        {
            down1_instant_ms = now;
            state = b2m_state::single_down;
        }
        else if (state == b2m_state::single_up)
        {
            down2_instant_ms = now;
            state = b2m_state::double_down;
        }

        DO_CALLBACK(press_cb);
    }

    // Hold detection and reporting. We intentionally use the double-click window duration here.
    if ((state == b2m_state::single_down) &&
        (now >= (down1_instant_ms + doubleclick_duration_ms)))
    {
        state = b2m_state::single_hold;
        DO_CALLBACK(single_hold_cb);
    }
    else if ((state == b2m_state::double_down) &&
             (now >= (down2_instant_ms + doubleclick_duration_ms)))
    {
        state = b2m_state::double_hold;
        DO_CALLBACK(double_hold_cb);
    }
}

/////////////////////////////////////////////////////////////////

void Butt2Mod::_whenUp(unsigned long now)
{
    // If we're going from pressed to released.
    if (prev_level == _activeLevel)
    {
        // Debouncing strategy: ignore level changes for the debounce time.
        // If a genuine button press happens during the debounce window then
        // it will just be deferred until the end of the window.
        debouncing_end_instant_ms = now + debounce_duration_ms;

        if (state == b2m_state::single_down)
        {
            state = b2m_state::single_up;
        }
        else if (state == b2m_state::double_down)
        {
            state = b2m_state::double_up;
        }
        // Hold-ended detection and reporting.
        else if ((state == b2m_state::single_hold) ||
                 (state == b2m_state::double_hold))
        {
            state = b2m_state::idle;
            DO_CALLBACK(hold_ended_cb);
        }
    }

    // Single/double-click detection and reporting.
    if ((state == b2m_state::single_up) &&
        (now >= (down1_instant_ms + doubleclick_duration_ms)))
    {
        state = b2m_state::idle;
        DO_CALLBACK(single_click_cb);
    }
    else if (state == b2m_state::double_up)
    {
        state = b2m_state::idle;
        DO_CALLBACK(double_click_cb);
    }
}

/////////////////////////////////////////////////////////////////

uint8_t Butt2Mod::_getLevel() const
{
    if (get_level_fn != NULL)
    {
        return get_level_fn();
    }
    else
    {
        return digitalRead(pin);
    }
}

/////////////////////////////////////////////////////////////////
