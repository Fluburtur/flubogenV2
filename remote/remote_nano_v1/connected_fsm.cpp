/* The buttons used on the remote are:
 *   Index  Label  IO pin  Port
 *     0     LH1     D8     PB0
 *     1     LH2     D7     PD7
 *     2     LH3     D6     PD6
 *     3     LH4     D5     PD5
 *     4     RH1     A1     PC1
 *     5     RH2     A0     PC0
 *     6     RH3     D12    PB4
 *     7     RH4     D11    PB3
 *
 *
 * 1. When pressing a single button, play command animations 1 to 8 (there are 8 buttons).
 * 2. When pressing several buttons at once (we call that "chording"), play command animations 9 to
 *    whatever. The upper limit of animations is only bounded by how many unique button press
 *    combinations there are.
 * 3. When holding a button or chord, the selected animation will keep playing on repeat and can't
 *    be interrupted until the buttons are released.
 * 4. When an animation is started by the remote it always plays to completion.
 * 5. When double clicking any button, lock the idle animation as it currently is so it cant be
 *    interrupted or change brightness until double clicking again.
 * 6. When a remote-triggered animation finishes playing, go back to the idle animation.
 *
 * We use a button library for debouncing and distinguishing between single-click/hold/double-click
 * etc. See the module-level comments in Butt2Mod.h for more info. Note that this introduces a
 * small delay between performing a single-click and the library reporting it.
 * A double-click requires two presses within a 300 ms window. A hold requires the button to be
 * held for at least 300 ms.
 */

#include <Arduino.h>

#include "config.h"

#include "Butt2Mod.h"
#include "connected_fsm.h"
#include "messages.h"
#include "ButtonsBits.h"

/***********************
 * Defines and types
 ***********************/

/* If this changes we'll have to update ButtonsBits (it's currently hard-coded for 8 buttons). */
#define NUM_BUTTONS 8

enum press_state_t
{
    PRESS_STATE_IDLE,
    PRESS_STATE_PLAYING_REPEAT,
    PRESS_STATE_SINGLE_CHORD_START,
    PRESS_STATE_REPEAT_CHORD_START,
    PRESS_STATE_IGNORE_UNTIL_ALL_RELEASED,
    PRESS_STATE_LOCKED,
};

/***********************
 * Data
 ***********************/

static const uint8_t button_idx_to_pin[NUM_BUTTONS] = {8, 7, 6, 5, A1, A0, 12, 11};

/***********************
 * Variables
 ***********************/

static Butt2Mod buttons[NUM_BUTTONS];

/* Tracks that we expect a click/hold event soon. */
static ButtonsBits future_events;
static ButtonsBits single_clicks;
static ButtonsBits single_holds;
static ButtonsBits double_clicks;
static ButtonsBits double_holds;

static press_state_t press_state;
static bool locked;

#ifdef CONNECTED_FSM_DEBUGGING
/* These store the ButtonsBits values from the previous iteration.
 * This lets us know when something has changed, and so when to print. */
uint8_t prev_future_events;
uint8_t prev_single_clicks;
uint8_t prev_single_holds;
uint8_t prev_double_clicks;
uint8_t prev_double_holds;
#endif

/***********************
 * Function prototypes
 ***********************/

static void update_buttons();
static void cb_future_event(Butt2Mod &btn);
static void cb_single_click(Butt2Mod &btn);
static void cb_single_hold(Butt2Mod &btn);
static void cb_double_click(Butt2Mod &btn);
static void cb_double_hold(Butt2Mod &btn);
static void cb_hold_ended(Butt2Mod &btn);
static void clear_click_tracking();

#ifdef CONNECTED_FSM_DEBUGGING
static void do_interactive();
static void print_buttons_bits(char label1, char label2, ButtonsBits &bits);
#endif

static void do_idle();

/***********************
 * Public functions
 ***********************/

void connected_fsm_setup()
{
#ifdef CONNECTED_FSM_DEBUGGING
    Serial.println("FSM setup");
#endif

    clear_click_tracking();

    for (uint8_t i = 0; i < NUM_BUTTONS; i++)
    {
        Butt2Mod &btn = buttons[i];
        btn.reset();
        btn.setID(i);
        btn.setPressHandler(cb_future_event);
        btn.setSingleClickHandler(cb_single_click);
        btn.setSingleHoldHandler(cb_single_hold);
        btn.setDoubleClickHandler(cb_double_click);
        btn.setDoubleHoldHandler(cb_double_hold);
        btn.setHoldEndedHandler(cb_hold_ended);
        btn.begin(button_idx_to_pin[i]);
    }

    press_state = PRESS_STATE_IDLE;
    locked = false;
}

void connected_fsm_do_work()
{
    /* The Butt2Mod callbacks (with the exception of Press) report events like click, hold,
     * double-click, etc. after enough time has passed to make the user's intent unambiguous. This
     * introduces a small delay. For example: when the user does a single-click, Butt2Mod has to
     * wait until the double-click window passes so it can tell the difference between a
     * single-click and a double-click.
     *
     * This is useful, because we don't have to add complexity here to disambiguate the different
     * kinds of press/hold. But the delay is a bit of a problem, especially when considering
     * multiple buttons.
     * For example, think about the case where the user is trying to single-click a two-button
     * chord. They won't begin the presses at exactly the same time, so at some instant in time
     * Butt2Mod will report a single-click from the earlier button but not the later button.
     *
     * We'll deal with this in two ways:
     *   - Use the Press callback to let us know that a click or hold event will arrive soon.
     *   - When a click or hold event first arrives, start a short chording window and collect any
     *     click and hold events that arrive within the window. If there is more than one event,
     *     treat this as a chord, otherwise a non-chord.
     */

    /* Causes the various cb_ functions to be called when there is button activity. */
    update_buttons();

#ifdef CONNECTED_FSM_DEBUGGING
    do_interactive();
#else

    switch (press_state)
    {
    case PRESS_STATE_IDLE:
        do_idle();
        break;

    default:
        break;
    }
#endif
}

/***********************
 * Private functions
 ***********************/

static void update_buttons()
{
    for (uint8_t i = 0; i < NUM_BUTTONS; i++)
    {
        Butt2Mod &btn = buttons[i];
        btn.loop();
    }
}

static void cb_future_event(Butt2Mod &btn)
{
    // A press has occurred -- button has gone unpressed to pressed and isn't bouncing.
    // That means we'll get one of the click/hold events soon.

    future_events.set(btn.getID());
}

static void cb_single_click(Butt2Mod &btn)
{
    int id = btn.getID();
    future_events.clear(id);
    single_clicks.set(id);
}

static void cb_single_hold(Butt2Mod &btn)
{
    int id = btn.getID();
    future_events.clear(id);
    single_holds.set(id);
}

static void cb_double_click(Butt2Mod &btn)
{
    int id = btn.getID();
    future_events.clear(id);
    double_clicks.set(id);
}

static void cb_double_hold(Butt2Mod &btn)
{
    int id = btn.getID();
    future_events.clear(id);
    double_holds.set(id);
}

static void cb_hold_ended(Butt2Mod &btn)
{
    int id = btn.getID();
    single_holds.clear(id);
    double_holds.clear(id);
}

static void clear_click_tracking()
{
#ifdef CONNECTED_FSM_DEBUGGING
    Serial.println("Clear click tracking");
#endif

    future_events.clear_all();
    single_clicks.clear_all();
    single_holds.clear_all();
    double_clicks.clear_all();
    double_holds.clear_all();

#ifdef CONNECTED_FSM_DEBUGGING
    prev_future_events = future_events.get_raw();
    prev_single_clicks = single_clicks.get_raw();
    prev_single_holds = single_holds.get_raw();
    prev_double_clicks = double_clicks.get_raw();
    prev_double_holds = double_holds.get_raw();
#endif
}

#ifdef CONNECTED_FSM_DEBUGGING
static void do_interactive()
{
    uint8_t curr_future_events = future_events.get_raw();
    uint8_t curr_single_clicks = single_clicks.get_raw();
    uint8_t curr_single_holds = single_holds.get_raw();
    uint8_t curr_double_clicks = double_clicks.get_raw();
    uint8_t curr_double_holds = double_holds.get_raw();

    bool has_changed = ((curr_future_events != prev_future_events) ||
                        (curr_single_clicks != prev_single_clicks) ||
                        (curr_single_holds != prev_single_holds) ||
                        (curr_double_clicks != prev_double_clicks) ||
                        (curr_double_holds != prev_double_holds));

    if (has_changed)
    {
        print_buttons_bits(' ', 'F', future_events);
        print_buttons_bits(' ', '1', single_clicks);
        print_buttons_bits('H', '1', single_holds);
        print_buttons_bits(' ', '2', double_clicks);
        print_buttons_bits('H', '2', double_holds);
        Serial.println();
    }

    /* Click events are cleared immediately after printing. */
    single_clicks.clear_all();
    double_clicks.clear_all();

    prev_future_events = curr_future_events;
    prev_single_clicks = curr_single_clicks;
    prev_single_holds = curr_single_holds;
    prev_double_clicks = curr_double_clicks;
    prev_double_holds = curr_double_holds;
}
#endif

#ifdef CONNECTED_FSM_DEBUGGING
static void print_buttons_bits(char label1, char label2, ButtonsBits &bits)
{

    /* label1 char, label2 char, colon, 3 buttons, a space, \0 */
    char buf[1 + 1 + 1 + 3 + 1 + 1];
    buf[0] = label1;
    buf[1] = label2;
    buf[2] = ':';
    for (uint8_t btn_idx = 0; btn_idx < 3; btn_idx++)
    {
        if (bits.is_set(btn_idx))
        {
            buf[btn_idx + 3] = 'X';
        }
        else
        {
            buf[btn_idx + 3] = '-';
        }
    }
    buf[6] = ' ';
    buf[7] = 0;

    Serial.print(buf);
}
#endif

static void do_idle()
{
    // TODO
}
