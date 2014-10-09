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

#include <stdint.h> /* uint8_t */
struct Thing;



/* Build "t"'s field of view and update its map memory with the result. */
extern void build_fov_map(struct Thing * t);



#endif
