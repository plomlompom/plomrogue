/* src/client/array_append.c */

#include "array_append.h"
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* free() */
#include <string.h> /* memcpy() */
#include "../common/try_malloc.h" /* try_malloc() */



extern void array_append(uint32_t old_n, size_t region_size, void * new_region,
                        void ** ptr_old_array)
{
    uint32_t old_size = old_n * region_size;
    uint32_t new_size = old_size + region_size;
    char * new_array = try_malloc(new_size, __func__);
    memcpy(new_array, * ptr_old_array, old_size);
    memcpy(new_array + (old_n * region_size), new_region, region_size);
    free(* ptr_old_array);
    * ptr_old_array = new_array;
}

