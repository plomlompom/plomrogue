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
    CLEANUP_FIFO            = 0x0001,
    CLEANUP_OUTFILE         = 0x0002,
    CLEANUP_MAP_OBJECT_DEFS = 0x0004,
    CLEANUP_MAP_OBJECTS     = 0x0008,
    CLEANUP_MAP_OBJECT_ACTS = 0x0010
};

extern void set_cleanup_flag(enum cleanup_flag flag);

/* Frees memory and unlinks some files. */
extern void cleanup();



#endif
