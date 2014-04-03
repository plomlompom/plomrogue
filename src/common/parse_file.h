/* src/common/parse_file.h
 *
 * Tools for parsing config files.
 */

#ifndef PARSE_FILE_H
#define PARSE_FILE_H

#include <stdint.h> /* uint8_t */



/* Many functions working on config file lines / tokens work with these elements
 * that only change on line change. Makes sense to pass them over together.
 */
struct Context {
    char * line;
    char * token0;
    char * token1;
    char * err_pre;
};



enum parse_flags
{
    EDIT_STARTED  = 0x01
};



/* Writes "context"->token1 to "target" only if it describes a proper uint8. */
extern void set_uint8(struct Context * context, uint8_t * target);

/* If "context"->token0 fits "comparand", set "element" to value read from
 * ->token1 as either string (type: "s"), char ("c") or uint8 ("8"), set
 * that element's flag to "flags" and return 1; else return 0.
 */
extern uint8_t set_val(struct Context * context, char * comparand,
                       uint8_t * flags, uint8_t set_flag, char type,
                       char * element);

/* Parse file at "path" by passing each line's tokens to "token_to_entry"
 * encapsulated into "Context". Empty lines are ignored. Non-empty lines have to
 * feature exactly two tokens as delimited either be whitespace or single quotes
 * (to allow for tokens featuring whitespaces). When EOF is reached,
 * token_to_entry() is called a last time with an empty Context.token0.
 */
extern void parse_file(char * path, void ( *token_to_entry) (struct Context *));



#endif
