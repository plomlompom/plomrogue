/* src/server/map.h
 *
 * Struct for the game map and routines to create and scroll on it.
 */

#ifndef MAP_H
#define MAP_H

#include <stdint.h> /* uint8_t */
#include "../common/yx_uint8.h" /* yx_uint8 struct */
#include "../common/yx_uint16.h" /* yx_uint16 struct */



struct Map
{
    struct yx_uint16 size; /* Map's height/width (use max. 256x256)! */
    char * cells; /* Sequence of bytes encoding map cells. */
    uint8_t dist_orthogonal; /* Ratio of the diagonal movement penalty as   */
    uint8_t dist_diagonal;   /* encoded by (.dist_diagonal/.dist_orthonal). */
};



/* Initialize island map "~" cells representing water and "." cells representing
 * land. The shape of the island is generated randomly by starting with a sea
 * containing one land cell in the middle and then going into a cycle of
 * repeatedly selecting a random cell on the map and transforming it into a land
 * cell if it is horizontally or vertically neighbor to one; the cycle ends when
 * a land cell is due to be created right at the border of the map.
 */
extern void init_map();

/* Check if coordinate "pos" on (or beyond) world.map is accessible to map
 * object movement.
 */
extern uint8_t is_passable(struct yx_uint8 pos);



#endif
