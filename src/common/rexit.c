/* src/common/rexit.c */

#include "rexit.h"
#include <errno.h> /* global errno */
#include <stdint.h> /* uint16_t */
#include <stdio.h> /* printf(), perror(), sprintf() */
#include <stdlib.h> /* exit(), free(), EXIT_FAILURE */
#include <string.h> /* strlen() */
#include "try_malloc.h" /* try_malloc() */



void (* cleanup_func) ();



extern void set_cleanup_func(void (* f)())
{
    cleanup_func = f;
}



extern void exit_err(int err, char * msg)
{
    if (0 == err)
    {
        return;
    }
    cleanup_func();
    if (NULL == msg)
    {
        msg = "Details unknown.";
    }
    printf("Aborted program due to error. %s\n", msg);
    if (0 != errno)
    {
        perror("errno states");
    }
    exit(EXIT_FAILURE);
}



extern void exit_trouble(int err, char * parent, char * child)
{
    char * f_name = "exit_trouble()";
    char * p1 = "Trouble in ";
    char * p2 = " with ";
    char * p3 = ".";
    uint16_t size = strlen(p1) + strlen(parent) + strlen(p2) + strlen(child)
                    + strlen(p3) + 1;
    char * msg = try_malloc(size, f_name);
    int test = sprintf(msg, "%s%s%s%s%s", p1, parent, p2, child, p3);
    exit_err(test < 0, "Trouble in exit_trouble() with sprintf()");
    exit_err(err, msg);
    free(msg);
}
