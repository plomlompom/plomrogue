/* src/common/rexit.h
 *
 * Routines to exit the game orderly on error, with cleaning up and a somewhat
 * informative message.
 */

#ifndef REXIT_H
#define REXIT_H



/* Set "f" as the cleanup function to be called on exit. */
extern void set_cleanup_func(void (* f)());

/* If "err" == 0, do nothing. Else, clean up and exit with an error message that
 * consists, first, of "msg" or (if "msg" is a NULL pointer) a generic "Details
 * unknown", secondly of "err" as the "internal error code", and thirdly of
 * errno if it is non-zero.
 */
extern void exit_err(int err, char * msg);

/* Do exit_err() with "msg" as: "Trouble in ".parent." with ".child."." */
extern void exit_trouble(int err, char * parent, char * child);



#endif
