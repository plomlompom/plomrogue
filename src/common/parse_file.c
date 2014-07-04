/* src/common/parse_file.c */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "parse_file.h"
#include <stddef.h> /* size_t, NULL */
#include <stdio.h> /* FILE, snprintf() */
#include <stdint.h> /* int16_t,uint8_t,uint32_t, INT16_MIN, UINT{8,16,32}_MAX */
#include <stdlib.h> /* atoi(), free() */
#include <string.h> /* strchr, strcmp(), strdup(), strlen() */
#include <unistd.h> /* access(), F_OK */
#include "readwrite.h" /* try_fopen(), try_fclose(), textfile_width() */
#include "rexit.h" /* exit_err(), exit_trouble() */
#include "try_malloc.h" /* try_malloc() */



/* Set by parse_file(), helps err_line() deciding what to do/output on error. */
static uint32_t err_line_count = 0;
static char * err_line_line = NULL;
static char * err_line_intro = NULL;
static uint8_t err_line_exit = 1;



/* Determines the end of the token_from_line() token. */
static void set_token_end(char ** start, char ** limit_char);



static void set_token_end(char ** start, char ** limit_char)
{
    char * end_quote = ('\'' == (* start)[0]) ? strchr(* start + 1, '\''): NULL;
    * start = (end_quote) ? * start + 1 : *start;
    if (end_quote)
    {
        * end_quote = '\0';
        * limit_char = end_quote;
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



extern void parse_file(char * path, void (* token_to_entry) (char *, char *))
{
    char * f_name = "read_new_config_file()";
    char * prefix = "Failed reading config file: \"";
    char * affix = "\". ";
    size_t size = strlen(prefix) + strlen(path) + strlen(affix) + 1;
    char * errline_intro = try_malloc(size, f_name);
    int test = snprintf(errline_intro, size, "%s%s%s", prefix, path, affix);
    exit_trouble(test < 0, f_name, "snprintf()");
    exit_err(access(path, F_OK), errline_intro);
    FILE * file = try_fopen(path, "r", f_name);
    uint32_t linemax = textfile_width(file);
    char * errline_line = try_malloc(linemax + 1, f_name);
    set_err_line_options(errline_intro, errline_line, 0, 1);
    err_line(0 == linemax, "File is empty.");
    char * token0 = NULL; /* For final token_to_entry() if while() stagnates. */
    char * token1 = NULL;
    char * err_val = "No value given.";
    while (try_fgets(errline_line, linemax + 1, file, f_name))
    {
        err_line_count++;
        err_line(UINT32_MAX == err_line_count, "Line reaches max lines limit.");
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
    try_fclose(file, f_name);
    free(errline_line);
    free(errline_intro);
}



extern void set_err_line_options(char * intro, char * line, uint32_t count,
                                 uint8_t exit)
{
    err_line_count = count;
    err_line_line = line;
    err_line_intro = intro;
    err_line_exit = exit;
}



extern uint8_t err_line(uint8_t test, char * msg)
{
    if (!test)
    {
        return 0;
    }
    char * f_name = "err_line()";
    char * prefix = " Offending line ";
    char * affix = ":\n";
    size_t size =   strlen(err_line_intro) + strlen(msg) + strlen(prefix)
                  + 10                 /* strlen for uint32_t representations */
                  + strlen(affix) + strlen(err_line_line) + 1;
    char * err = try_malloc(size, f_name);
    int ret = snprintf(err, size, "%s%s%s%d%s%s", err_line_intro, msg, prefix,
                       err_line_count, affix, err_line_line);
    exit_trouble(ret < 0, f_name, "snprintf()");
    if (err_line_exit)
    {
        exit_err(1, err);
    }
    exit_trouble(0 > printf("%s\n", err), f_name, "printf()");
    exit_trouble(EOF == fflush(stdout), f_name, "fflush()");
    free(err);
    return 1;
}



extern char * token_from_line(char * line)
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
    if (final_char < start)
    {
        return NULL;
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
        return NULL;
    }
    set_token_end(&start, &limit_char);
    return start;
}



extern uint8_t parsetest_int(char * string, char type)
{
    char * err_8 = "Value must represent proper unsigned 8 bit integer.";
    char * err_i = "Value must represent proper signed 16 bit integer.";
    char * err_u = "Value must represent proper unsigned 16 bit integer.";
    char * err_U = "Value must represent proper unsigned 32 bit integer.";
    char * err = ('8' == type) ? err_8 : err_U;
    err = ('i' == type) ? err_i : err;
    err = ('u' == type) ? err_u : err;
    uint8_t ret = err_line(strlen(string) < 1, err);
    uint8_t i;
    uint8_t test;
    for (i = 0; '\0' != string[i]; i++)
    {
        char * err_many = "Value of too many characters.";
        ret = ret + err_line(string[i + 1] && UINT8_MAX == i, err_many);
        test = (   (0 == i && ('-' == string[i] || '+' == string[i]))
                || ('0' <= string[i]  && string[i] <= '9'));
        ret = ret + err_line(!test, err);
    }
    ret = ret + err_line(   strlen(string) < 2
                         && ('-' == string[i] || '+' == string[i]), err);
    test =     (   '8' == type
                && (   strlen(string) > 4
                    || atoi(string) < 0 || atoi(string) > UINT8_MAX))
            || (   'i' == type
                && (   strlen(string) > 6
                    || atol(string) < INT16_MIN || atol(string) > INT16_MAX))
            || (   'u' == type
                && (   strlen(string) > 6
                    || atoll(string) < 0 || atol(string) > UINT16_MAX))
            || (   'U' == type
                && (   strlen(string) > 11
                    || atoll(string) < 0 || atoll(string) > UINT32_MAX));
    ret = ret + err_line(test, err);
    return ret;
}



extern void parsetest_defcontext(uint8_t flags)
{
    err_line(!(flags & EDIT_STARTED),"Outside appropriate definition context.");
}



extern uint8_t parsetest_singlechar(char * string)
{
    return err_line(1 !=strlen(string),"Value must be single ASCII character.");
}



extern void parsetest_too_many_values()
{
    err_line(NULL != token_from_line(NULL), "Too many values.");
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
    char * f_name = "parse_init_entry()";
    *flags = EDIT_STARTED;
    char * p = try_malloc(size, f_name);
    memset(p, 0, size);
    return p;
}



extern uint8_t parse_val(char * token0, char * token1, char * comparand,
                         char type, char * element)
{
    if (!strcmp(token0, comparand))
    {
        if      ('s' == type)
        {
            * (char **) element = strdup(token1);
        }
        else if ('c' == type && !parsetest_singlechar(token1))
        {
            *element = (token1)[0];
        }
        else if (!parsetest_int(token1, type))
        {
            if ('8' == type)
            {
                * (uint8_t *) element = atoi(token1);
            }
            else if ('i' == type)
            {
                * (int16_t *) element = atoi(token1);
            }
            else if ('u' == type)
            {
                * (uint16_t *) element = atol(token1);
            }
            else if ('U' == type)
            {
                * (uint32_t *) element = atoll(token1);
            }
        }
        return 1;
    }
    return 0;
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
