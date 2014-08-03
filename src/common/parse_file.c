/* src/common/parse_file.c */

#define _POSIX_C_SOURCE 200809L /* strdup(), snprintf() */
#include "parse_file.h"
#include <stddef.h> /* size_t, NULL */
#include <stdio.h> /* FILE, snprintf() */
#include <stdint.h> /* int16_t,uint8_t,uint32_t, INT16_MIN, UINT{8,16,32}_MAX */
#include <stdlib.h> /* atoi(), free() */
#include <string.h> /* strchr, strcmp(), strdup(), strlen() */
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



extern void set_err_line_options(char * intro, char * line, uint8_t exit)
{
    err_line_line = line;
    err_line_intro = intro;
    err_line_exit = exit;
}



extern void err_line_inc()
{
    err_line_count++;
    err_line(UINT32_MAX == err_line_count, "Line reaches max lines limit.");
}



extern void err_line_zero()
{
    err_line_count = 0;
}



extern uint8_t err_line(uint8_t test, char * msg)
{
    if (!test)
    {
        return 0;
    }
    char * prefix = " Offending line ";
    char * affix = ": ";
    size_t size =   strlen(err_line_intro) + strlen(msg) + strlen(prefix)
                  + 10                 /* strlen for uint32_t representations */
                  + strlen(affix) + strlen(err_line_line) + 1;
    char * err = try_malloc(size, __func__);
    int ret = snprintf(err, size, "%s%s%s%d%s%s", err_line_intro, msg, prefix,
                       err_line_count, affix, err_line_line);
    exit_trouble(ret < 0, __func__, "snprintf");
    if (err_line_exit)
    {
        exit_err(1, err);
    }
    exit_trouble(0 > printf("%s\n", err), __func__, "printf");
    exit_trouble(EOF == fflush(stdout), __func__, "fflush");
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



extern uint8_t parsetest_singlechar(char * string)
{
    return err_line(1 !=strlen(string),"Value must be single ASCII character.");
}



extern uint8_t parse_val(char * token0, char * token1, char * comparand,
                         char type, char * element)
{
    if (!strcmp(token0, comparand))
    {
        if      ('s' == type)
        {
            free(* (char **) element);
            * (char **) element = strdup(token1);
        }
        else if ('c' == type)
        {
            if (!parsetest_singlechar(token1))
            {
                *element = (token1)[0];
            }
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
