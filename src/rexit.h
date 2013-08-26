/* rexit.h
 *
 * Routines to exit the game orderly or on error, with as much cleaning up as is
 * possible in both cases.
 */

#ifndef REXIT_H
#define REXIT_H



#include <stdint.h> /* for uint8_t */
struct World;
struct Map;



/* set_cleanup_flag() sets any of the flags defined in cleanup_flag to announce
 * the resources that need cleaning up upon program exit. It is to be called at
 * the earliest moment possible after resource creation / initialization.
 */
enum cleanup_flag
{
    CLEANUP_NCURSES     = 0x01,
    CLEANUP_MAP         = 0x02,
    CLEANUP_KEYBINDINGS = 0x04,
    CLEANUP_LOG         = 0x08
};
extern void set_cleanup_flag(enum cleanup_flag flag);



/* Exit orderly, clean up. */
extern void exit_game(struct World * world);



/* If "err" == 0, do nothing. Else, clean up and exit with an error message that
 * consists, first, of "msg" or (if "msg" is a NULL pointer) a generic "Details
 * unknown, secondly of "err" as the "internal error code", and thirdly of errno
 * if it is non-zero.
 */
extern void exit_err(uint8_t err, struct World * world, char * msg);



#endif
