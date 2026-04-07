#ifndef CALYNDA_WII_IO_H
#define CALYNDA_WII_IO_H

/*
 * Wii console I/O support for the Calynda runtime.
 * Initializes libogc VIDEO and console subsystems so that
 * printf/puts in the runtime writes to the Wii framebuffer.
 */

/* Initialize the Wii video and console subsystem.
 * Must be called before any printf output.  After this call,
 * stdout is redirected to the libogc framebuffer console. */
void calynda_wii_console_init(void);

/* Block until the user powers off the Wii.  Call after
 * start() returns so Dolphin does not close immediately. */
void calynda_wii_wait_forever(void);

#endif /* CALYNDA_WII_IO_H */
