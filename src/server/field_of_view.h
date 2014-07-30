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

/* Build "eye"'s field of view. */
extern void build_fov_map(struct Thing * eye);



#endif
