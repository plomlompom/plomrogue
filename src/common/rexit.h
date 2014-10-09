/* src/common/rexit.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Routines to exit the game orderly on error, with cleaning up and a somewhat
 * informative message.
 */

#ifndef REXIT_H
#define REXIT_H



/* Set "f" as the cleanup function to be called on exit. */
extern void set_cleanup_func(void (* f)());

/* If "err" == 0, do nothing. Else, clean up and exit with an error message that
 * consists of "msg" or (if "msg" is NULL pointer) a generic "Details unknown"
 * and of errno's content (if it is non-zero).
 */
extern void exit_err(int err, const char * msg);

/* Do exit_err() with "msg" as: "Trouble in ".parent." with ".child."." */
extern void exit_trouble(int err, const char * parent, const char * child);



#endif
