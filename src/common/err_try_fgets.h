/* err_try_fgets.h
 *
 * For superficial syntax checks of individual text file lines.
 */

#ifndef ERR_TRY_FGETS_H
#define ERR_TRY_FGETS_H

#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* FILE */



/* Reset err_try_fgets() file line counter back to zero. */
extern void reset_err_try_fgets_counter();

/* Set delimiter expected by err_try_fgets()'s 'c' test. */
extern void set_err_try_fgets_delim(char * delim);

/* If "test", print error message of "intro" + "msg" and output offending line's
 * number and content.
 */
extern void err_line(uint8_t test, char * line, char * intro, char * msg);

/* fgets() / try_fgets() wrapper (therefore the common arguments "line",
 * "linemax", "file") that performs various checks as defined by characters in
 * "test". On failure, these tests exit the game with an error message that
 * pre-pends "context" to a description of the individual test failure and
 * output of the offending line's number and content.
 *
 * Note that for the file line count to be correct, it is necessary to call
 * reset_err_try_fgets_counter() before reading the line, and each line must be
 * read with a call of err_try_fgets().
 *
 * The available "test" conditions are as follows:
 *
 * '0': check against the line being empty (not even containing a \n char)
 * 'n': check for "line" ending with an \n char
 * 'e': check for "line" starting with an \n char
 * 'f': check for "line" not starting with an \n char
 * 's': check for "line" containing two chars (the second may be \n)
 * 'd': check for "line" being equal to the world.delim delimiter
 * 'i': check for "line" describing an integer in all its chars before end or \n
 *      (i.e. all other chars must be digits, except the first char, which may
 *       be '+' or '-'; a '+' or '-' without digits following is invalid)
 */
extern void err_try_fgets(char * line, uint32_t linemax, FILE * file,
                          char * context, char * test);



#endif
