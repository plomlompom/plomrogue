/* src/server/map.h
 *
 * Struct for the game map and routines to create and scroll on it.
 */

#ifndef MAP_H_SERVER
#define MAP_H_SERVER

#include <stdint.h> /* uint8_t */
#include "../common/map.h" /* struct Map */
#include "../common/yx_uint16.h" /* yx_uint16 struct */



/* Initialize island map as 64 x 64 "~" cells representing water and "." cells
 * representing land. The shape of the island is generated randomly by starting
 * with a sea containing one land cell in the middle and then going into a cycle
 * of repeatedly selecting a random cell on the map and transforming it into a
 * land cell if it is horizontally or vertically neighbor to one; the cycle ends
 * when a land cell is due to be created right at the border of the map.
 */
extern void init_map();

/* Check if coordinate "pos" on (or beyond) world.map is accessible to map
 * object movement.
 */
extern uint8_t is_passable(struct yx_uint16 pos);



#endif
