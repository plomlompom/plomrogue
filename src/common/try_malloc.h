/* src/common/try_malloc.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * malloc() wrapper.
 */

#ifndef TRY_MALLOC_H
#define TRY_MALLOC_H

#include <sys/types.h> /* for size_t */



/* Call malloc("size") from function called "f"; exit_trouble() on error. */
extern void * try_malloc(size_t size, const char * f);



#endif
