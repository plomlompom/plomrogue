/* src/client/misc.c */

#include "misc.h"
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdlib.h> /* free() */
#include <string.h> /* memcpy() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "world.h" /* global world */



extern void nav_inventory(char dir)
{
    if ('u' == dir)
    {
        world.player_inventory_select = world.player_inventory_select
                                        - (world.player_inventory_select > 0);
        return;
    }
    uint8_t n_elems = 0;
    uint8_t i;
    for (i = 0; '\0' != world.player_inventory[i]; i++)
    {
        n_elems = n_elems + ('\n' == world.player_inventory[i]);
    }
    world.player_inventory_select = world.player_inventory_select
                                    + (world.player_inventory_select < n_elems);
}



extern void array_append(uint32_t old_n, size_t region_size, void * new_region,
                        void ** ptr_old_array)
{
    char * f_name = "array_append()";
    uint32_t old_size = old_n * region_size;
    uint32_t new_size = old_size + region_size;
    char * new_array = try_malloc(new_size, f_name);
    memcpy(new_array, * ptr_old_array, old_size);
    memcpy(new_array + (old_n * region_size), new_region, region_size);
    free(* ptr_old_array);
    * ptr_old_array = new_array;
}
