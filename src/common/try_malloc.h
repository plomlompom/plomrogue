/* src/common/try_malloc.h
 *
 * malloc() wrapper.
 */

#ifndef TRY_MALLOC_H
#define TRY_MALLOC_H

#include <sys/types.h> /* for size_t */



/* Call malloc("size") from function called "f"; exit_trouble() on error. */
extern void * try_malloc(size_t size, const char * f);



#endif
