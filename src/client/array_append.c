/* src/client/array_append.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "array_append.h"
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t, UINT32_MAX */
#include <stdlib.h> /* free() */
#include <string.h> /* memcpy() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */



extern void array_append(uint32_t old_n, size_t region_size, void * new_region,
                        void ** ptr_old_array)
{
    exit_trouble(UINT32_MAX<(old_n+1)*region_size, __func__, "too large sizes");
    uint32_t old_size = old_n * region_size;
    uint32_t new_size = old_size + region_size;
    char * new_array = try_malloc(new_size, __func__);
    memcpy(new_array, * ptr_old_array, old_size);
    memcpy(new_array + (old_n * region_size), new_region, region_size);
    free(* ptr_old_array);
    *ptr_old_array = new_array;
}
