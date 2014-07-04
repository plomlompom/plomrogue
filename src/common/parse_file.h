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
 * "token_to_entry". Ignore empty lines. Non-empty lines must feature at least
 * two tokens as delimited either be whitespace or single quotes (to allow for
 * tokens featuring whitespaces). When EOF is reached, token_to_entry() is
 * called a last time with a first token of NULL.
 */
extern void parse_file(char * path, void ( *token_to_entry) (char *, char *));

/* Set err_line() options: "intro" message, char array used to store analyzed
 * lines ("line"), line start "count", whether to "exit" on error message.
 */
extern void set_err_line_options(char * intro, char * line, uint32_t count,
                                 uint8_t exit);

/* If "test", output "msg", faulty line, its number and exit if so defined by
 * set_err_line_options(), else return 1 on "test" and 0 if "test" is 0.
 */
extern uint8_t err_line(uint8_t test, char * msg);

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

/* Test for "string" to represent proper int16 (type: "i"), uint8 ("8"), uint16
 * ("u") or uint32 ("U"). Returns 0 if proper value, else >0.
 */
extern uint8_t parsetest_int(char * string, char type);

/* Test for "string" to be of length 1 (excluding "\0"). Return 1 on failure. */
extern uint8_t parsetest_singlechar(char * string);

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
 * string (type: "s"), char ("c") uint8 ("8"), uint16 ("u"), uint32 ("U") or
 * int16 ("i"), and return 1; else 0.
 */
extern uint8_t parse_val(char * token0, char * token1, char * comparand,
                         char type, char * element);

/* Wrapper to parse_val() that sets "flags" to "flags"|"set_flag" on success. */
extern uint8_t parse_flagval(char * token0, char * token1, char * comparand,
                             uint8_t * flags, uint8_t set_flag, char type,
                             char * element);

/* Check "ready_flag" is set in "flags", re-set "flags" to "ready_flag" only. */
extern void parse_and_reduce_to_readyflag(uint8_t * flags, uint8_t ready_flag);



#endif
