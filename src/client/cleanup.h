/* src/client/cleanup.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Stuff defining / performing the cleanup called by rexit.h's exit functions.
 */

#ifndef CLEANUP_H
#define CLEANUP_H



/* set_cleanup_flag() sets any of the flags defined in cleanup_flag to announce
 * the resources that need cleaning up upon program exit. It is to be called at
 * the earliest moment possible after resource creation / initialization.
 */
enum cleanup_flag
{
    CLEANUP_NCURSES    = 0x0001,
    CLEANUP_INTERFACE  = 0x0002,
    CLEANUP_COMMANDS   = 0x0004,
    CLEANUP_SERVER_IN  = 0x0008,
    CLEANUP_SERVER_OUT = 0x0008
};

extern void set_cleanup_flag(enum cleanup_flag flag);

/* Frees memory and properly winds down ncurses / resets the terminal. */
extern void cleanup();



#endif
