/* src/client/array_append.h
 *
 * Small memory management helper.
 */

#ifndef ARRAY_APPEND_H
#define ARRAY_APPEND_H

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t */



/* Append to array pointed to by "ptr_old_array" of "old_n" elements of
 * "region_size" "new region".
 */
extern void array_append(uint32_t old_n, size_t region_size, void * new_region,
                         void ** ptr_old_array);



#endif

