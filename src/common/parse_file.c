/* src/common/parse_file.c */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "parse_file.h"
#include <stddef.h> /* NULL */ // size_t
#include <stdio.h> /* FILE, sprintf() */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdlib.h> /* free() */ // atoi()
#include <string.h> /* strchr, strdup(), strlen() */ // strcmp()
#include <unistd.h> /* access(), F_OK */
#include "err_try_fgets.h" /* err_line(), err_try_fgets(),
                            * reset_err_try_fgets_counter()
                            */
#include "readwrite.h" /* try_fopen(),try_fclose(), textfile_width() */
#include "rexit.h" /* exit_err() */
#include "try_malloc.h" /* try_malloc() */



/* Return next token from "line" or NULL if none is found. Tokens either a)
 * start at the first non-whitespace character and end before the next
 * whitespace character after that; or b) if the first non-whitespace character
 * is a single quote followed by at least one other single quote some time later
 * on the line, the token starts after that first single quote and ends before
 * the second, with the next token_from_line() call starting its token search
 * after that second quote. The only way to return an empty string (instead of
 * NULL) as a token is to delimit the token by two succeeding single quotes.
 * */
static char * token_from_line(char * line);

/* Determines the end of the token_from_line() token. */
static void set_token_end(char ** start, char ** limit_char);



static char * token_from_line(char * line)
{
    static char * final_char = NULL;
    static char * limit_char = NULL;
    char * start = limit_char + 1;
    if (line)
    {
        start      = line;
        limit_char = start;
        final_char = &(line[strlen(line)]);
        if ('\n' == *(final_char - 1))
        {
            *(--final_char) = '\0';
        }
    }
    uint8_t empty = 1;
    uint32_t i;
    for (i = 0; '\0' != start[i]; i++)
    {
        if (' ' != start[i] && '\t' != start[i])
        {
            start = &start[i];
            empty = 0;
            break;
        }
    }
    if (empty)
    {
        return start = NULL;
    }
    set_token_end(&start, &limit_char);
    return start;
}



static void set_token_end(char ** start, char ** limit_char)
{
    char * end_quote = ('\'' == (*start)[0]) ? strchr(*start + 1, '\'') : NULL;
    *start = (end_quote) ? *start + 1 : *start;
    if (end_quote)
    {
        *end_quote = '\0';
        *limit_char = end_quote;
        return;
    }
    char * space = strchr(*start, ' ');
    char * tab   = strchr(*start, '\t');
    space = (!space || (tab && tab < space)) ? tab : space;
    if (space)
    {
        * space = '\0';
    }
    *limit_char = strchr(*start, '\0');
}



extern void set_uint8(struct Context * context, uint8_t * target)
{
    char * err_uint8 = "Value not unsigned decimal number between 0 and 255.";
    uint8_t i;
    uint8_t is_uint8 = 1;
    for (i = 0; '\0' != context->token1[i]; i++)
    {
        if (i > 2 || '0' > context->token1[i] || '9' < context->token1[i])
        {
            is_uint8 = 0;
        }
    }
    if (is_uint8 && atoi(context->token1) > UINT8_MAX)
    {
        is_uint8 = 0;
    }
    err_line(!(is_uint8), context->line, context->err_pre, err_uint8);
    *target = atoi(context->token1);
}



extern uint8_t set_val(struct Context * context, char * comparand,
                       uint8_t * flags, uint8_t set_flag, char type,
                       char * element)
{
    char * err_out = "Outside appropriate definition's context.";
    char * err_singlechar = "Value must be single ASCII character.";
    if (!strcmp(context->token0, comparand))
    {
        err_line(!(* flags & EDIT_STARTED),
                 context->line, context->err_pre, err_out);
        * flags = * flags | set_flag;
        if      ('s' == type)
        {
            * (char **) element = strdup(context->token1);
        }
        else if ('c' == type)
        {
            err_line(1 != strlen(context->token1),
                     context->line, context->err_pre, err_singlechar);
            * element = (context->token1)[0];
        }
        else if ('8' == type)
        {
            set_uint8(context, (uint8_t *) element);
        }
        return 1;
    }
    return 0;
}



extern void parse_file(char * path, void (* token_to_entry) (struct Context *))
{
    char * f_name = "read_new_config_file()";
    struct Context context;
    char * err_pre_prefix = "Failed reading config file: \"";
    char * err_pre_affix = "\". ";
    context.err_pre = try_malloc(strlen(err_pre_prefix) + strlen(path)
                                 + strlen(err_pre_affix) + 1, f_name);
    sprintf(context.err_pre, "%s%s%s", err_pre_prefix, path, err_pre_affix);
    exit_err(access(path, F_OK), context.err_pre);
    FILE * file = try_fopen(path, "r", f_name);
    uint32_t linemax = textfile_width(file);
    context.line = try_malloc(linemax + 1, f_name);
    reset_err_try_fgets_counter();
    err_line(0 == linemax, context.line, context.err_pre, "File is empty.");
    context.token0 = NULL;      /* For token_to_entry() if while() stagnates. */
    char * err_val = "No value given.";
    char * err_many = "Too many values.";
    while (err_try_fgets(context.line, linemax + 1, file, context.err_pre, ""))
    {
        char * line_copy = strdup(context.line);
        context.token0 = token_from_line(line_copy);
        if (context.token0)
        {
            err_line(0 == (context.token1 = token_from_line(NULL)),
                     context.line, context.err_pre, err_val);
            err_line(NULL != token_from_line(NULL),
                     context.line, context.err_pre, err_many);
            token_to_entry(&context);
            context.token0 = NULL;
        }
        free(line_copy);
    }
    token_to_entry(&context);
    try_fclose(file, f_name);
    free(context.line);
    free(context.err_pre);
}
