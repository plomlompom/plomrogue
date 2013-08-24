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



/* If "err" != 0, exit with an error message "msg" and clean up. If "msg" is a
 * NULL pointer, return generic error message. (For "err", pass the result of
 * functions that return non-zero as an error status and thereby avoid bloating
 * up the code with if-error-conditionals.)
 */
extern void exit_err(uint8_t err, struct World * world, char * msg);



#endif
