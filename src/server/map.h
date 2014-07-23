/* src/server/map.h
 *
 * Routines to create and navigate game map.
 */

#ifndef MAP_H_SERVER
#define MAP_H_SERVER

#include <stdint.h> /* uint8_t, uint16_t */
#include "../common/yx_uint8.h" /* yx_uint8 struct */



/* (Re-)make island map "~" cells representing water and "." cells representing
 * land. The island shape is built randomly from world.seed_map by starting with
 * a sea of one land cell in the middle, then going into a cycle of repeatedly
 * selecting a random sea cell and transforming it into land if it is neighbor
 * to land; the cycle ends when a land cell is due to be created right at the
 * border of the map. Lots of 'X' cells representing trees are put on the
 * island.
 */
extern void remake_map();

/* Check if coordinate "pos" on (or beyond) world.map is accessible to thing
 * movement.
 */
extern uint8_t is_passable(struct yx_uint8 pos);



#endif
