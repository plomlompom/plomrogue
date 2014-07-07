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

/* Return field of view map of the world as seen from the position of "eye". */
extern uint8_t * build_fov_map(struct Thing * eye);



#endif
