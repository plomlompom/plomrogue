/* src/server/cleanup.h
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
    CLEANUP_FIFO          = 0x0001,
    CLEANUP_WORLDSTATE    = 0x0002,
    CLEANUP_THING_TYPES   = 0x0004,
    CLEANUP_THINGS        = 0x0008,
    CLEANUP_THING_ACTIONS = 0x0010,
    CLEANUP_IN            = 0x0020,
    CLEANUP_OUT           = 0x0040
};

/* In addition, unset_cleanup_flag() may be used to unset flags. */
extern void set_cleanup_flag(enum cleanup_flag flag);
extern void unset_cleanup_flag(enum cleanup_flag flag);

/* Frees memory and unlinks some files. */
extern void cleanup();



#endif
