# User Requirements

This is written for the one existing remote hardware design, which uses buttons:

1. When pressing a single button, play command animations 1 to 8 (there are 8 buttons).
1. When pressing several buttons at once, play command animations 9 to whatever, the amount of animations is arbitrary based on how many inputs you can press at once.
1. When staying pressed on a button, the selected animation will keep playing on repeat and cant be interrupted until the button is released.
1. When an animation is started it always plays to completion.
1. When double clicking any button, lock the idle animation as it currently is so it cant be interrupted or change brightness until double clicking again.
1. When a remote-triggered animation finishes playing, go back to the idle animation.

But more generally, we'd like the system to work with different kinds of remote -- more buttons, fewer buttons, touchscreen, wireless... To that end, a simple and general-purpose protocol is desirable.

# Remote Design

As mentioned, the current remote hardware design has 8 buttons. Button debouncing must be done in software.

# Protocol

Between the remote and the brain.

At the physical layer, communication is via UART using 115200 8N1.

At the message layer, a message consists of a 1 byte command followed by 0 or more parameter bytes.

On power up, the remote must send only the "hello" message once every second until a reply is received.
The "hello" message specifies which remote protocol version should be used.
The brain must listen for the "hello" message and respond appropriately.

The brain may send the "reset" message at any time.
When the remote receives the "reset" message, it must act as if it has just powered up: clear all state, go back to sending only the "hello" message.

### Hello

Direction: remote to brain  
Response: version-specific

| Command byte | Version (1 byte) |
| ------------ | ---------------- |
| 0xE1         | 1-255            |

#### Intent

The remote should not send a "hello" message after receiving a response to the "hello".

The brain should be prepared to receive a "hello" message at any time, in case the remote crashes and reboots.

### Reset

Direction: brain to remote  
Response: none

The remote must clear its state and go back to sending "hello" messages.

| Command byte |
| ------------ |
| 0xE2         |

#### Intent

When the brain boots, it should wait 10 seconds for the remote to send a "hello" message.
If this does not happen, then either:
* The remote is not functioning.
* The remote has already started, but the brain has crashed and rebooted.

In either case the brain should send a "reset" command to the remote every 10 seconds,
in an attempt to restore communications between them.

The brain may also choose to send the "reset" command as a form of error handling.

## Version 1

The response to the "hello" message is the "start" message.
After the brain sends the "start" message to the remote, the remote is free to send any other remote-to-brain messages.

### Start

Direction: brain to remote  
Response: none

The remote begins normal operation.

| Command byte |
| ------------ |
| 0xE3         |

### Play Animation Once

Direction: remote to brain  
Response: none

Causes the brain to play the chosen animation one time.
After playback completes, the brain returns to the idle animation.

If no such animation exists then the command is ignored.
If an animation is still playing from a previous command then the command is ignored.

| Command byte | Animation Number (1 byte) |
| ------------ | ------------------------- |
| 0x01         | 1-255                     |

### Play Animation Repeat

Direction: remote to brain  
Response: none

Causes the brain to play the chosen animation on repeat. Use "end animation" to stop this.

If no such animation exists then the command is ignored.
If a different animation is still playing from a previous command then the command is ignored.

| Command byte | Animation Number (1 byte) |
| ------------ | ------------------------- |
| 0x02         | 1-255                     |

### End Animation

Direction: remote to brain  
Response: none

Causes the brain to finish the current loop of "play animation repeat".
After playback completes, the brain returns to the idle animation.

If a repeated animation is not playing then the command is ignored.

| Command byte |
| ------------ |
| 0x03         |

### Lock Idle

Direction: remote to brain  
Response: none

Causes the brain to enter the idle animation, then stay there at the current brightness.
Other animation commands are ignored while in this state.
Use "unlock" to stop this.

If an animation is still playing from a previous command then the command is ignored.

| Command byte |
| ------------ |
| 0x04         |

#### Intent

Makes photogaphy easier.

### Unlock

Direction: remote to brain  
Response: none

Exit the "lock idle" state.

If the brain is not in the "lock idle" state then this command is ignored.

| Command byte |
| ------------ |
| 0x05         |
