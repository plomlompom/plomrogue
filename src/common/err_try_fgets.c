/* err_try_fgets.c */

#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include <string.h> /* strlen(), strchr(), strcmp() */
#include "../common/readwrite.h" /* try_fgets() */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */



/* Increments by one on each err_try_fgets() call, servers as a line counter. */
static uint32_t err_try_fgets_counter = 0;

/* Delimiter to use for err_try_fgets()' 'c' test. */
char * err_try_fgets_delim = "";



extern void reset_err_try_fgets_counter()
{
    err_try_fgets_counter = 0;
}



extern void set_err_try_fgets_delim(char * delim)
{
    err_try_fgets_delim = delim;
}



extern void err_line(uint8_t test, char * line, char * intro, char * msg)
{
    if (!test)
    {
        return;
    }
    char * f_name = "err_line()";
    char * line_intro = " Offending line ";
    char * err = try_malloc(strlen(intro) + strlen(msg) + strlen(line_intro) +
                            10 + 1 + 1 + strlen(line) + 1, f_name);
    sprintf(err, "%s%s%s%d:\n%s", intro, msg, line_intro, err_try_fgets_counter,
                                 line);
    exit_err(1, err);
}



extern void err_try_fgets(char * line, uint32_t linemax, FILE * file,
                          char * context, char * test)
{
    char * err_end   = "File ended unexpectedly.";
    char * err_empty = "Hit empty line where non-empty line was expected.";
    char * err_many  = "Too many characters; expected only one.";
    char * err_int   = "Expected valid positive or negative integer number.";
    char * err_full  = "Hit non-empty line where empty line was expected.";
    char * err_delim = "Expected proper delimiter, found something else.";
    char * f_name = "err_try_fgets()";
    line[0] = '\0';
    try_fgets(line, linemax + 1, file, f_name);
    err_try_fgets_counter++;
    err_line(strchr(test, '0') && !(strlen(line)), line, context, err_end);
    err_line(strchr(test, 'n') && line[strlen(line) - 1] != '\n', line, context,
             err_end);
    err_line(strchr(test, 'e') && '\n' != line[0], line, context, err_full);
    err_line(strchr(test, 'f') && '\n' == line[0], line, context, err_empty);
    err_line(strchr(test, 's') && strlen(line) > 2, line, context, err_many);
    err_line(strchr(test, 'd') && strcmp(line, err_try_fgets_delim), line,
             context, err_delim);
    if (strchr(test, 'i'))
    {
        err_line(!(strchr(test, 'f')) && strlen(line) < 2, line, context,
                 err_int);
        uint8_t i;
        for (i = 0; '\n' != line[i] && '\0' != line[i]; i++)
        {
            uint8_t test =    (0 == i && ('-' == line[i] || '+' == line[i]))
                           || ('0' <= line[i] && line[i] <= '9');
            err_line(!test, line, context, err_int);
        }
        err_line(strlen(line) < 2 && ('-' == line[i] || '+' == line[i]),
                 line, context, err_int);
    }
}
