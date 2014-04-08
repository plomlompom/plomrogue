/* src/common/parse_file.h
 *
 * Tools for parsing config files.
 */

#ifndef PARSE_FILE_H
#define PARSE_FILE_H

#include <stddef.h> /* size_t */
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
extern void parsetest_int(char * string, char type);

/* Test for "string" to be of length 1 (excluding "\0" terminator). */
extern void parsetest_singlechar(char * string);

/* Calls err_line() with fitting message if EDIT_STARTED not set in "flags". */
extern void parsetest_defcontext(uint8_t flags);

/* Ensure token_from_line() does not find any more tokens on the line. */
extern void parsetest_too_many_values();

/* Trigger err_line() with "Unknown argument" message. */
extern void parse_unknown_arg();

/* If "test"!=0 call err_line() with "Declaration of ID already used" message.*/
extern void parse_id_uniq(int test);

/* Set "flags"=EDIT_STARTED and return pointer to new zeroed array of "size". */
extern char * parse_init_entry(uint8_t * flags, size_t size);

/* If "token0" fits "comparand", set "element" to value read from "token1" as
 * string (type: "s"), char ("c") uint8 ("8") or int16 ("i"), set that element's
 * flag to "flags" and return 1; else return 0.
 */
extern uint8_t parse_val(char * token0, char * token1, char * comparand,
                         uint8_t * flags, uint8_t set_flag, char type,
                         char * element);

/* Check "ready_flag" is set in "flags", re-set "flags" to "ready_flag" only. */
extern void parse_and_reduce_to_readyflag(uint8_t * flags, uint8_t ready_flag);



#endif
