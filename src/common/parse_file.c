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



/* Set by parse_file(), used by err_line() for more informative messages. */
static uint32_t err_line_count = 0;
static char * err_line_line = NULL;
static char * err_line_intro = NULL;



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
    err_line_intro = try_malloc(size, f_name);
    int test = snprintf(err_line_intro, size, "%s%s%s", prefix, path, affix);
    exit_trouble(test < 0, f_name, "snprintf()");
    exit_err(access(path, F_OK), err_line_intro);
    FILE * file = try_fopen(path, "r", f_name);
    uint32_t linemax = textfile_width(file);
    err_line_line = try_malloc(linemax + 1, f_name);
    err_line_count = 0;
    err_line(0 == linemax, "File is empty.");
    char * token0 = NULL; /* For final token_to_entry() if while() stagnates. */
    char * token1;
    char * err_val = "No value given.";
    while (try_fgets(err_line_line, linemax + 1, file, f_name))
    {
        err_line_count++;
        err_line(UINT32_MAX == err_line_count, "Line reaches max lines limit.");
        char * line_copy = strdup(err_line_line);
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
    free(err_line_line);
    free(err_line_intro);
}



extern void err_line(uint8_t test, char * msg)
{
    if (!test)
    {
        return;
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
    exit_err(1, err);
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



extern void test_for_int(char * string, char type)
{
    char * err;
    if ('8' == type)
    {
        err = "Value must be proper representation of unsigned 8 bit integer.";
    }
    if ('i' == type)
    {
        err = "Value must be proper representation of signed 16 bit integer.";
    }
    err_line(strlen(string) < 1, err);
    uint8_t i;
    uint8_t test;
    for (i = 0; '\0' != string[i]; i++)
    {
        char * err_many = "Value of too many characters.";
        err_line(string[i + 1] && UINT8_MAX == i, err_many);
        test = (   (0 == i && ('-' == string[i] || '+' == string[i]))
                || ('0' <= string[i]  && string[i] <= '9'));
        err_line(!test, err);
    }
    err_line(strlen(string) < 2 && ('-' == string[i] || '+' == string[i]), err);
    err_line('8'==type && (atoi(string) < 0 || atoi(string) > UINT8_MAX), err);
    test = 'i'==type && (atoi(string) < INT16_MIN || atoi(string) > INT16_MAX);
    err_line(test, err);
}



extern uint8_t set_val(char * token0, char * token1, char * comparand,
                       uint8_t * flags, uint8_t set_flag, char type,
                       char * element)
{
    if (!strcmp(token0, comparand))
    {
        char * err_out = "Outside appropriate definition's context.";
        char * err_singlechar = "Value must be single ASCII character.";
        err_line(!(*flags & EDIT_STARTED), err_out);
        *flags = *flags | set_flag;
        if      ('s' == type)
        {
            * (char **) element = strdup(token1);
        }
        else if ('c' == type)
        {
            err_line(1 != strlen(token1), err_singlechar);
            *element = (token1)[0];
        }
        else if ('8' == type)
        {
            test_for_int(token1, '8');
            * (uint8_t *) element = atoi(token1);
        }
        else if ('i' == type)
        {
            test_for_int(token1, 'i');
            * (int16_t *) element = atoi(token1);
        }
        return 1;
    }
    return 0;
}
