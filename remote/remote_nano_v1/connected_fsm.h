#pragma once

/* Handles the majority of the remote's logic once it has connected to the brain. */

/** Set up (or reset) the FSM. */
void connected_fsm_setup();

/**
 * Read the buttons and send commands to the brain when needed.
 *
 * Call this repeatedly from the main loop.
 */
void connected_fsm_do_work();
