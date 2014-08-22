/* src/server/map.h
 *
 * Routines to create and navigate game map.
 */

#ifndef MAP_H_SERVER
#define MAP_H_SERVER

#include <stdint.h> /* uint8_t */
struct yx_uint8;



/* (Re-)make island map "~" cells representing water and "." cells representing
 * land. The island shape is built randomly from world.seed_map by starting with
 * a sea of one land cell in the middle, then going into a cycle of repeatedly
 * selecting a random sea cell and transforming it into land if it is neighbor
 * to land; the cycle ends when a land cell is due to be created right at the
 * border of the map. Lots of 'X' cells representing trees are put on the
 * island.
 */
extern void remake_map();

/* Wrapper to mv_yx_in_dir_wrap(), returns 1 if the wrapped function moved "yx"
 * within the wrap borders and the map size, else 0.
 */
extern uint8_t mv_yx_in_dir_legal(char dir, struct yx_uint8 * yx);


#endif
