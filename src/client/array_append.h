/* src/client/array_append.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
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

