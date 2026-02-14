/* Communication transport to the remote control.
 * Internal to the remote module. */

#ifndef _REMOTE_MSG_H_
#define _REMOTE_MSG_H_

/**
 * Initialise the remote_msg module.
 */
void remote_msg_init(void);

/**
 * Send the "reset" command.
 */
void remote_msg_send_reset(void);

/**
 * Send the "start" command.
 */
void remote_msg_send_start(void);

#endif /* _REMOTE_MSG_H_ */
