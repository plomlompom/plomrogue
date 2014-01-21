/* src/common/try_malloc.c */

#include "try_malloc.h"
#include <stdlib.h> /* for malloc */
#include <sys/types.h> /* for size_t */
#include "rexit.h" /* for exit_trouble() */



extern void * try_malloc(size_t size, char * f)
{
    void * p = malloc(size);
    exit_trouble(NULL == p, f, "malloc()");
    return p;
}
