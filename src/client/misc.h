/* src/client/misc.h
 *
 * Miscellaneous routines that have not yet found a proper parent module. Having
 * LOTS of stuff in here is a sure sign that better modularization is in order.
 */

#ifndef MISC_H
#define MISC_H

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t */



/* Move world.inventory_sel up ("dir"="u") or down (else) as far as possible. */
extern void nav_inventory(char dir);

/* Append to array pointed to by "ptr_old_array" of "old_n" elements of
 * "region_size" "new region".
 */
extern void array_append(uint32_t old_n, size_t region_size, void * new_region,
                         void ** ptr_old_array);



#endif
