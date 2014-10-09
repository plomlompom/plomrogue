/* src/common/try_malloc.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "try_malloc.h"
#include <stdlib.h> /* for malloc */
#include <stdio.h> /* sprintf() */
#include <string.h> /* strlen() */
#include <sys/types.h> /* for size_t */
#include "rexit.h" /* for exit_err() */



extern void * try_malloc(size_t size, const char * f)
{
    char * prefix = "Trouble with malloc in ";
    char * msg = malloc(strlen(prefix) + strlen(f) + 1 + 1);
    exit_err(!msg,
             "Trouble in try_malloc with malloc for error message string.");
    int test = sprintf(msg, "%s%s.", prefix, f);
    exit_err(test < 0, "Trouble in try_malloc with sprintf.");
    void * p = malloc(size);
    exit_err(!p, msg);         /* Bypass exit_trouble() calling try_malloc(). */
    free(msg);
    return p;
}
