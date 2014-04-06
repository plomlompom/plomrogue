/* src/common/parse_file.h
 *
 * Tools for parsing config files.
 */

#ifndef PARSE_FILE_H
#define PARSE_FILE_H

#include <stdint.h> /* uint8_t */



enum parse_flags
{
    EDIT_STARTED  = 0x01
};



/* Parse file at "path" by passing each line's first two tokens to
 * "token_to_entry". Ignore empty line. Non-empty lines must feature at least
 * two tokens as delimited either be whitespace or single quotes (to allow for
 * tokens featuring whitespaces). When EOF is reached, token_to_entry() is
 * called a last time with a first token of NULL.
 */
extern void parse_file(char * path, void ( *token_to_entry) (char *, char *));

/* If "test" != 0, exit on output of "msg" and faulty line and line number as
 * parsed by parse_file(). (Ought to be called as offspring to parse_file().)
 */
extern void err_line(uint8_t test, char * msg);

/* Return next token from "line" or NULL if none is found. Tokens either a)
 * start at the first non-whitespace character and end before the next
 * whitespace character after that; or b) if the first non-whitespace character
 * is a single quote followed by at least one other single quote some time later
 * on the line, the token starts after that first single quote and ends before
 * the second, with the next token_from_line() call starting its token search
 * after that second quote. The only way to return an empty string (instead of
 * NULL) as a token is to delimit the token by two succeeding single quotes.
 * */
extern char * token_from_line(char * line);

/* Test for "string" to represent proper int16 (type: "i") or uint8 ("8"). */
extern void test_for_int(char * string, char type);

/* If "token0" fits "comparand", set "element" to value read from "token1" as
 * string (type: "s"), char ("c") uint8 ("8") or int16 ("i"), set that element's
 * flag to "flags" and return 1; else return 0.
 */
extern uint8_t set_val(char * token0, char * token1, char * comparand,
                       uint8_t * flags, uint8_t set_flag, char type,
                       char * element);



#endif
