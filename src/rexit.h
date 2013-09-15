/* rexit.h
 *
 * Routines to exit the game orderly or on error, with as much cleaning up as is
 * possible in both cases.
 */

#ifndef REXIT_H
#define REXIT_H



#include <stdint.h> /* for uint8_t */
struct World;



/* set_cleanup_flag() sets any of the flags defined in cleanup_flag to announce
 * the resources that need cleaning up upon program exit. It is to be called at
 * the earliest moment possible after resource creation / initialization.
 */
enum cleanup_flag
{
    CLEANUP_NCURSES         = 0x0001,
    CLEANUP_MAP             = 0x0002,
    CLEANUP_KEYBINDINGS     = 0x0004,
    CLEANUP_LOG             = 0x0008,
    CLEANUP_COMMAND_DB      = 0x0010,
    CLEANUP_MAP_OBJECTS     = 0x0020,
    CLEANUP_MAP_OBJECT_DEFS = 0x0040,
    CLEANUP_WIN_META        = 0x0080,
    CLEANUP_WINCONFS        = 0x0100
};
extern void set_cleanup_flag(enum cleanup_flag flag);



/* Exit orderly, clean up. */
extern void exit_game(struct World * world);



/* If "err" == 0, do nothing. Else, clean up and exit with an error message that
 * consists, first, of "msg" or (if "msg" is a NULL pointer) a generic "Details
 * unknown", secondly of "err" as the "internal error code", and thirdly of
 * errno if it is non-zero.
 */
extern void exit_err(uint8_t err, struct World * world, char * msg);



#endif
