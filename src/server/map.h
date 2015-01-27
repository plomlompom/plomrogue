/* src/server/map.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
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

/* Move "yx" into hex direction "dir". Available hex directions are: 'e'
 * (north-east), 'd' (east), 'c' (south-east), 'x' (south-west), 's' (west), 'w'
 * (north-west). Returns 1 if the move was legal, else 0.
 *
 * A move is legal if "yx" ends up in the confines of the map and the original
 * wrap space. The latter is left to a neighbor wrap space if "yx" moves beyond
 * the minimal (0) or maximal (UINT8_MAX) column or row of possible map space –
 * in which case "yx".y or "yx".x will snap to the respective opposite side. The
 * current wrapping state is kept between successive calls until a "yx" of NULL
 * is passed, in which case the function does nothing but zero the wrap state.
 * Successive wrapping may move "yx" several wrap spaces into either direction,
 * or return it into the original wrap space.
 */
extern uint8_t mv_yx_in_dir_legal(char dir, struct yx_uint8 * yx);

/* Initialize (empty) map array at "map". */
extern void init_empty_map(char ** map);


#endif
