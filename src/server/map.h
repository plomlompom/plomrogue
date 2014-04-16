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
};



/* Initialize island map "~" cells representing water and "." cells representing
 * land. The island shape is built randomly by starting with a sea of one land
 * cell in the middle, then going into a cycle of repeatedly selecting a random
 * seal cell and transforming it into land if it is neighbor to land; the cycle
 * ends when a land cell is due to be created right at the border of the map.
 */
extern void init_map();

/* Check if coordinate "pos" on (or beyond) world.map is accessible to map
 * object movement.
 */
extern uint8_t is_passable(struct yx_uint8 pos);



#endif
