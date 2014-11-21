/* src/common/parse_file.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Tools for parsing files.
 */

#ifndef PARSE_FILE_H
#define PARSE_FILE_H

#include <stdint.h> /* uint8_t */



/* Set err_line() options: "intro" message, char array used to store analyzed
 * lines ("line"), and whether to "exit" on error message.
 */
extern void set_err_line_options(char * intro, char * line, uint8_t exit);

/* Increment and reset (to zero) err_line() line counter. */
extern void err_line_inc();
extern void err_line_zero();

/* If "test", output "msg", faulty line, its number and exit if so defined by
 * set_err_line_options(), else return 1 on "test" and 0 if "test" is 0.
 */
extern uint8_t err_line(uint8_t test, char * msg);

/* Return next token from "line", or NULL if none is found. Tokens either a)
 * start at the first non-whitespace character and end before the next
 * whitespace character after that; or b) if the first non-whitespace character
 * is a single quote followed by at least one other single quote some time later
 * on the line, the token starts after that first single quote and ends before
 * the second, with the next token_from_line() call starting its token search
 * after that second quote. The only way to return an empty string (instead of
 * NULL) as a token is to delimit the token by two succeeding single quotes.
 */
extern char * token_from_line(char * line);

/* Test for "string" to represent proper int16 (type: "i"), uint8 ("8"), uint16
 * ("u") or uint32 ("U"). Returns 0 if proper value, else >0.
 */
extern uint8_t parsetest_int(char * string, char type);

/* Test for "string" to be of length 1 (excluding "\0"). Return 1 on failure. */
extern uint8_t parsetest_singlechar(char * string);

/* If "token0" fits "comparand", set "element" to value read from "token1" as
 * string (type: "s"), char ("c") uint8 ("8"), uint16 ("u"), uint32 ("U") or
 * int16 ("i"), and return 1; else 0.
 */
extern uint8_t parse_val(char * token0, char * token1, char * comparand,
                         char type, char * element);



#endif
