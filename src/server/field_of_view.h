/* src/server/field_of_view.h
 *
 * Generate field of view maps.
 */



#ifndef FIELD_OF_VIEW_H
#define FIELD_OF_VIEW_H

#include <stdint.h> /* uint8_t */
struct MapObj;



/* States that cells in the fov map may be in. */
enum fov_cell_states {
    VISIBLE      = 0x01,
    HIDDEN       = 0x02,
    SHADOW_LEFT  = 0x04,
    SHADOW_RIGHT = 0x08,
    LIMIT        = 0x10,
    HIDE_LATER   = 0x20
};

/* Return overlay of world map wherein all cell positions visible from player's
 * positions have flag VISIBLE set.
 *
 * This is achieved by spiraling out clock-wise from the player position,
 * flagging cells as VISIBLE unless they're already marked as HIDDEN, and, on
 * running into obstacles for view that are not HIDDEN, casting shadows from
 * these, i.e. drawing cells as HIDDEN that would be hidden by said obstacle,
 * before continuing the original spiraling path.
 *
 * Shadowcasting during spiraling is initially lazy, flagging only the shadows'
 * interior cells as HIDDEN and their border cells as HIDE_LATER. Only at the
 * end are all cells flagged HIDE_LATER flagged as HIDDEN. This is to handle
 * cases where obstacles to view sit right at the border of pre-estabilshed
 * shadows, therefore might be ignored if HIDDEN and not cast shadows on their
 * own that may slightly extend beyond the pre-established shadows they border.
 */
extern uint8_t * build_fov_map(struct MapObj * eye);



#endif
