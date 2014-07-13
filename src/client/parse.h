/* src/client/parse.h
 *
 * Routines for file parsing.
 */

#ifndef PARSE_H
#define PARSE_H

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

/* Wrapper to parse_val() that sets "flags" to "flags"|"set_flag" on success. */
extern uint8_t parse_flagval(char * token0, char * token1, char * comparand,
                             uint8_t * flags, uint8_t set_flag, char type,
                             char * element);

/* Check "ready_flag" is set in "flags", re-set "flags" to "ready_flag" only. */
extern void parse_and_reduce_to_readyflag(uint8_t * flags, uint8_t ready_flag);



#endif

