/* src/server/field_of_view.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Generate field of view maps.
 */

#ifndef FIELD_OF_VIEW_H
#define FIELD_OF_VIEW_H

#include <stdint.h> /* uint8_t, uint32_t */
struct Thing;



/* Update "t"'s .mem_map memory with what's in its current FOV, remove from its
 * .t_mem all memorized things in FOV and add inanimate things in FOV to it.
 */
extern void update_map_memory(struct Thing * t_eye);

/* Build "t"'s field of view. */
extern void build_fov_map(struct Thing * t);



#endif
