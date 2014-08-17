/* src/client/parse.c */

#define _POSIX_C_SOURCE 200809L /* strdup(), snprintf() */
#include "parse.h"
#include <stddef.h> /* size_t, NULL */
#include <stdio.h> /* FILE, snprintf() */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdlib.h> /* free() */
#include <string.h> /* strdup(), strlen() */
#include <unistd.h> /* access(), F_OK */
#include "../common/parse_file.h" /* set_err_line_options(), err_line_inc(),
                                   * err_line_zero(), token_from_line()
                                   */
#include "../common/readwrite.h" /* try_fopen(),try_fclose(),textfile_width() */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */



extern void parse_file(char * path, void (* token_to_entry) (char *, char *))
{
    char * prefix = "Failed reading config file: \"";
    char * affix = "\". ";
    size_t size = strlen(prefix) + strlen(path) + strlen(affix) + 1;
    char * errline_intro = try_malloc(size, __func__);
    int test = snprintf(errline_intro, size, "%s%s%s", prefix, path, affix);
    exit_trouble(test < 0, __func__, "snprintf");
    exit_err(access(path, F_OK), errline_intro);
    FILE * file = try_fopen(path, "r", __func__);
    uint32_t linemax = textfile_width(file);
    char * errline_line = try_malloc(linemax + 1, __func__);
    set_err_line_options(errline_intro, errline_line, 1);
    err_line_zero();
    err_line(!linemax, "File is empty.");
    char * token0 = NULL; /* For final token_to_entry() if while() stagnates. */
    char * token1 = NULL;
    char * err_val = "No value given.";
    while (try_fgets(errline_line, linemax + 1, file, __func__))
    {
        err_line_inc();
        char * line_copy = strdup(errline_line);
        token0 = token_from_line(line_copy);
        if (token0)
        {
            err_line(0 == (token1 = token_from_line(NULL)), err_val);
            token_to_entry(token0, token1);
            token0 = NULL;
        }
        free(line_copy);
    }
    token_to_entry(token0, token1);
    try_fclose(file, __func__);
    free(errline_line);
    free(errline_intro);
}



extern void parsetest_defcontext(uint8_t flags)
{
    err_line(!(flags & EDIT_STARTED),"Outside appropriate definition context.");
}



extern void parsetest_too_many_values()
{
    err_line(!(!token_from_line(NULL)), "Too many values.");
}



extern void parse_id_uniq(int test)
{
    err_line(0 != test, "Declaration of ID already used.");
}



extern void parse_unknown_arg()
{
    err_line(1, "Unknown argument.");
}



extern char * parse_init_entry(uint8_t * flags, size_t size)
{
    *flags = EDIT_STARTED;
    char * p = try_malloc(size, __func__);
    memset(p, 0, size);
    return p;
}



extern uint8_t parse_flagval(char * token0, char * token1, char * comparand,
                             uint8_t * flags, uint8_t set_flag, char type,
                             char * element)
{
    if (parse_val(token0, token1, comparand, type, element))
    {
        parsetest_defcontext(*flags);
        *flags = *flags | set_flag;
        return 1;
    }
    return 0;
}



extern void parse_and_reduce_to_readyflag(uint8_t * flags, uint8_t ready_flag)
{
    char * err_fin = "Last definition block not finished yet.";
    err_line((*flags & ready_flag) ^ ready_flag, err_fin);
    *flags = ready_flag;
}
