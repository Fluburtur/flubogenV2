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

/* These definitions are for use with the the `button_map` below.
 * Turns human names into a value that can be used with ButtonsBits. */
#define LH1 (1 << 0)
#define LH2 (1 << 1)
#define LH3 (1 << 2)
#define LH4 (1 << 3)
#define RH1 (1 << 4)
#define RH2 (1 << 5)
#define RH3 (1 << 6)
#define RH4 (1 << 7)

#define ARRAY_NUM_ELEMS(arr) (sizeof(arr)/sizeof(arr[0]))

enum chording_state_t
{
    CHORDING_STATE_IDLE,
    CHORDING_STATE_COLLECTING,
};

enum action_t
{
    ACTION_NONE,
    ACTION_PLAYING_REPEAT,
    ACTION_WAIT_UNTIL_ALL_RELEASED,
};

typedef struct
{
    uint8_t animation_number;
    uint8_t button_bits;
} button_map_t;

/***********************
 * Data
 ***********************/

static const uint8_t button_idx_to_pin[NUM_BUTTONS] = {8, 7, 6, 5, A1, A0, 12, 11};

/* Says what button combination will activate each animation number. */
static const button_map_t button_map[] = {
    /* Single-button animations. */
    {1, LH1},
    {2, LH2},
    {3, LH3},
    {4, LH4},
    {5, RH1},
    {6, RH2},
    {7, RH3},
    {8, RH4},
    /* Multi-button animations. Combine the buttons with a single | character. */
    {9, LH1 | LH2},
    {10, LH1 | RH4}
};

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

static ButtonsBits collected_single_clicks;
static ButtonsBits collected_single_holds;
static ButtonsBits collected_double_clicks;
static ButtonsBits collected_double_holds;

static action_t current_action;
static chording_state_t chording_state;
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
static void execute_collection();
static void do_playing_repeat();
static void do_wait_until_all_released();

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

    current_action = ACTION_NONE;
    chording_state = CHORDING_STATE_IDLE;
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
     * We'll deal with this by using the Press callback, which lets us know that a click or hold
     * event will arrive soon (within Butt2Mod's double-click window, currently 300 ms).
     *   - The Press callback sets a bit in `future_events`, and the bit is cleared once the
     *     corresponding click/hold event arrives.
     *   - When a click or hold event first arrives, collect it and all further click/hold events
     *     until `future_events` is empty.
     * This has the effect of combining click/hold events into a chord, as long as each part of the
     * chord arrives within 300 ms of any other part of the chord.
     * 
     * For example, in the fastest case a two-button chord can be input instantaneously and
     * recognised in 300 ms. Or in the slowest case the chord can be input across 300 ms and
     * recognised in 600 ms, as in this diagram:
     *
     *     user click 1                   user click 2
     *        |                              |
     *        v                              v
     *        ------------300ms----------------
     *        |    click 1 disambiguation     |
     *        ---------------------------------
     *        |                              ------------300ms----------------
     *        |                              |    click 2 disambiguation     |
     *        |                              ---------------------------------
     *        |                              ||                              |
     *        v                              v|                              v
     *    future_events        future_events  |              click 2 reported,
     *    bit 1 is set         bit 2 is set   |              future_events
     *                                        |              bit 2 is cleared
     *                                        v
     *                                     click 1 reported,
     *                                     future_events
     *                                     bit 1 is cleared
     */

    /* Causes the various cb_ functions to be called when there is button activity. */
    update_buttons();

#ifdef CONNECTED_FSM_DEBUGGING
    do_interactive();
#endif

    switch (current_action)
    {
    case ACTION_NONE:
        do_idle();
        break;

    case ACTION_PLAYING_REPEAT:
        do_playing_repeat();
        break;

    case ACTION_WAIT_UNTIL_ALL_RELEASED:
        do_wait_until_all_released();
        break;
    }

    /* Click events don't persist between interations. */
    single_clicks.clear_all();
    double_clicks.clear_all();
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

    collected_single_clicks.clear_all();
    collected_single_holds.clear_all();
    collected_double_clicks.clear_all();
    collected_double_holds.clear_all();
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

/* Either nothing is happening, or we're receiving button press/click events and we're waiting for
 * them to stop. */
static void do_idle()
{
    collected_single_clicks.set_raw(single_clicks.get_raw());
    collected_single_holds.set_raw(single_holds.get_raw());
    collected_double_clicks.set_raw(double_clicks.get_raw());
    collected_double_holds.set_raw(double_holds.get_raw());

    switch (chording_state)
    {
    case CHORDING_STATE_IDLE:
        if (single_clicks.is_any_set() ||
            single_holds.is_any_set() ||
            double_clicks.is_any_set() ||
            double_holds.is_any_set())
        {
            chording_state = CHORDING_STATE_COLLECTING;
        }
        else
        {
            /* No clicks or holds. Nothing to do this iteration. */
            return;
        }
        /* Fall through */
        /* if there was at least one click or hold. */

    case CHORDING_STATE_COLLECTING:
        if (future_events.is_all_clear())
        {
            /* The press/click events have stopped. Act on them. */
            execute_collection();
        }
        break;
    }
}

static void execute_collection()
{
    /* A double-click on just one button. Toggle locked/unlocked and early return. */
    int double_click_btn_idx;
    if (collected_single_clicks.is_all_clear() &&
        collected_single_holds.is_all_clear() &&
        collected_double_clicks.get_single_set(double_click_btn_idx) &&
        collected_double_holds.is_all_clear())
    {
        /* TODO: this requires a re-design of the brain-remote protocol.
         * For now we'll just ignore it. */
#ifdef CONNECTED_FSM_DEBUGGING
        Serial.println("EXEC 2click TODO!");
#endif
        clear_click_tracking();
        current_action = ACTION_NONE;
        chording_state = CHORDING_STATE_IDLE;
        return;
    }

    /* Everything else plays an animation by looking up the collected button combination in the
     * `button_map`. But there is a bit of nuance to increase usability and deal with some edge
     * cases. */

    /* If there are any double-clicks we treat them as a single-click. This accounts for the user
     * accidentally double-clicking, or if their finger slipped, etc.
     * Likewise we treat double-holds as single-holds. */
    collected_single_clicks.set_raw(collected_double_clicks.get_raw());
    collected_single_holds.set_raw(collected_double_holds.get_raw());

    /* If there are any holds, we treat any clicks as holds too. This assumes the user wanted to
     * do a hold but their finger slipped.
     * TODO: maybe be a bit more clever? E.g. if there is a mix of holds and click, treat them as
     * whichever group has the most? */
    ButtonsBits final_collection;
    bool do_hold = collected_single_holds.is_any_set();
    if (do_hold)
    {
        final_collection.set_raw(collected_single_clicks.get_raw());
        final_collection.set_raw(collected_single_holds.get_raw());
    }
    else
    {
        final_collection.set_raw(collected_single_clicks.get_raw());
    }

    /* Now we check if the button combination has an associated animation. */
    uint8_t animation_number;
    bool found = false;
    for (size_t i = 0; i < ARRAY_NUM_ELEMS(button_map); i++)
    {
        button_map_t entry = button_map[i];
        if (entry.button_bits == final_collection.get_raw())
        {
            found = true;
            animation_number = entry.animation_number;
            break;
        }
    }

    if (found)
    {
        if (do_hold)
        {
            /* Holding button(s). Play animation on repeat until they let go. */
#ifdef CONNECTED_FSM_DEBUGGING
            Serial.println("EXEC hold");
#endif
            send_message_play_animation_repeat(animation_number);
            current_action = ACTION_PLAYING_REPEAT;
        }
        else
        {
#ifdef CONNECTED_FSM_DEBUGGING
            Serial.println("EXEC 1click");
#endif
            send_message_play_animation_once(animation_number);
            clear_click_tracking();
            current_action = ACTION_NONE;
            chording_state = CHORDING_STATE_IDLE;
        }
    }
    else
    {
#ifdef CONNECTED_FSM_DEBUGGING
        Serial.println("EXEC no match");
#endif
        if (do_hold)
        {
            /* If anything is held, wait until all released. */
#ifdef CONNECTED_FSM_DEBUGGING
            Serial.println("EXEC wait release");
#endif
            current_action = ACTION_WAIT_UNTIL_ALL_RELEASED;
        }
        else
        {
            /* Nothing held, can immediately begin listening for presses again. */
            clear_click_tracking();
            current_action = ACTION_NONE;
            chording_state = CHORDING_STATE_IDLE;
        }
    }
}

/* "Play Animation Repeat" has been sent and buttons are held.
 * When the buttons are released, stop the animation. */
static void do_playing_repeat()
{
    if (single_clicks.is_all_clear() &&
        single_holds.is_all_clear() &&
        double_clicks.is_all_clear() &&
        double_holds.is_all_clear() &&
        future_events.is_all_clear())
    {
#ifdef CONNECTED_FSM_DEBUGGING
        Serial.println("EXEC released");
#endif
        send_message_end_animation();
        clear_click_tracking();
        current_action = ACTION_NONE;
        chording_state = CHORDING_STATE_IDLE;
    }
}

/* Nothing useful is happening, but buttons are held.
 * When the buttons are released we can begin listening for presses again. */
static void do_wait_until_all_released()
{
    if (single_clicks.is_all_clear() &&
        single_holds.is_all_clear() &&
        double_clicks.is_all_clear() &&
        double_holds.is_all_clear() &&
        future_events.is_all_clear())
    {
#ifdef CONNECTED_FSM_DEBUGGING
        Serial.println("EXEC released");
#endif
        clear_click_tracking();
        current_action = ACTION_NONE;
        chording_state = CHORDING_STATE_IDLE;
    }
}
