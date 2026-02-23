/////////////////////////////////////////////////////////////////
/*
  Butt2Mod.h - Arduino Library to simplify working with buttons.
  Based on the Button2 library by Lennart Hennigs
  https://github.com/LennartHennigs/Button2 at commit 9ca66f2bb91c8ca78f45eed376b027d9e9bf3501

  We only care about reporting a few events:
  * Press (debounced press)
    Reported when the button goes from unpressed to pressed and the switch isn't bouncing.
  * Single click (press and release)
    Reported once the button has been released and the double-click window has passed.
  * Single hold (press and hold)
    Reported as soon as the button has been held for the duration of the double-click window.
  * Double click (press and release, press and release)
    Reported once the second button has been released.
  * Double hold (press and release, press and hold)
    Reported as soon as the second button has been held for the duration of the double-click window.
  * Hold ended (after finally releasing a single hold or double hold).

  "Progress" through the click states is reset after a hold-ended, or after a double-click. So for
  example, this means that continuous rapid clicks get reported as multiple double-click events.


  Copyright (C) 2017-2025 Lennart Hennigs.
  Released under the MIT license.

*/
/////////////////////////////////////////////////////////////////

#pragma once

#ifndef Butt2Mod_h
#define Butt2Mod_h

/////////////////////////////////////////////////////////////////

#include <Arduino.h>

/////////////////////////////////////////////////////////////////

const unsigned int BTN_DEBOUNCE_MS = 40;
const unsigned int BTN_DOUBLECLICK_MS = 300;

const unsigned int BTN_UNDEFINED_PIN = 255;
const unsigned int BTN_VIRTUAL_PIN = 254;

/////////////////////////////////////////////////////////////////

enum b2m_state
{
    idle,
    single_down,
    single_hold,
    single_up,
    double_down,
    double_hold,
    double_up,
};

class Butt2Mod
{
protected:
    // Memory layout optimized for minimal padding
    // Ordered by size: pointers/callbacks first, then long, int, uint16_t, uint8_t, bool

    typedef void (*CallbackFunction)(Butt2Mod &);
    typedef uint8_t (*LevelCallbackFunction)();
    typedef void (*InitCallbackFunction)();

    // Function pointers (largest members on most platforms)
    LevelCallbackFunction get_level_fn = NULL;
    CallbackFunction press_cb = NULL;
    CallbackFunction single_click_cb = NULL;
    CallbackFunction single_hold_cb = NULL;
    CallbackFunction double_click_cb = NULL;
    CallbackFunction double_hold_cb = NULL;
    CallbackFunction hold_ended_cb = NULL;

    // unsigned long (4 bytes on most platforms)

    // The instant when the button when from unpressed to pressed for the first press.
    unsigned long down1_instant_ms = 0;
    // The instant when the button when from unpressed to pressed for the second press.
    unsigned long down2_instant_ms = 0;
    // The instant at which we should resume observing level changes.
    unsigned long debouncing_end_instant_ms = 0;

    // unsigned int / uint16_t (2 bytes)
    unsigned int debounce_duration_ms = BTN_DEBOUNCE_MS;
    unsigned int doubleclick_duration_ms = BTN_DOUBLECLICK_MS;

    // int (2-4 bytes depending on platform)
    int id;

    // uint8_t (1 byte each)
    uint8_t pin;
    uint8_t curr_level = HIGH;
    uint8_t prev_level = HIGH;
    // Denotes which IO level is the active (pressed) level.
    uint8_t _activeLevel;

    // Enums (typically 1 byte)
    b2m_state state = b2m_state::idle;

    void _whenDown(unsigned long now);
    void _whenUp(unsigned long now);
    void _setDefaultID();

public:
    Butt2Mod();
    Butt2Mod(uint8_t attachTo, uint8_t buttonMode = INPUT_PULLUP, bool activeLow = true);

    void begin(
        uint8_t attachTo, uint8_t buttonMode = INPUT_PULLUP, bool activeLow = true,
        InitCallbackFunction initCallback = NULL);

    void setDebounceDuration(unsigned int ms);
    void setDoubleClickDuration(unsigned int ms);

    unsigned int getDebounceDuration() const;
    unsigned int getDoubleClickDuration() const;
    uint8_t getPin() const;

    void reset();

    void setButtonLevelFunction(LevelCallbackFunction f);
    void setPressHandler(CallbackFunction f);
    void setSingleClickHandler(CallbackFunction f);
    void setSingleHoldHandler(CallbackFunction f);
    void setDoubleClickHandler(CallbackFunction f);
    void setDoubleHoldHandler(CallbackFunction f);
    void setHoldEndedHandler(CallbackFunction f);

    // Is the button currently pressed, once debouncing has been taking into account?
    bool isPressed() const;
    // Is the button currently pressed? No debouncing.
    bool isPressedRaw() const;
    void resetPressedState();

    int getID() const;
    void setID(int newID);

    bool operator==(const Butt2Mod &rhs) const;

    void loop();

private:
    static int _nextID;
    uint8_t _getLevel() const;
};

#endif /* #ifndef Butt2Mod_h */
