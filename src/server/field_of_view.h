/* src/server/field_of_view.h
 *
 * Generate field of view maps.
 */



#ifndef FIELD_OF_VIEW_H
#define FIELD_OF_VIEW_H

#include <stdint.h> /* uint8_t */
struct Thing;



/* States that cells in the field of view map may be in. */
enum fov_cell_states {
    HIDDEN = 0x00,
    VISIBLE = 0x01
};

/* Build "t"'s field of view and update its map memory with the result. */
extern void build_fov_map(struct Thing * t);



#endif
