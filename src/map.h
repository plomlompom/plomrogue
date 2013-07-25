/* map.h
 *
 * Struct for the game map and routines to manipulate it.
 */

#ifndef MAP_H
#define MAP_H



#include "yx_uint16.h" /* for yx_uint16 and dir enums */
struct Player;



struct Map
{
    struct yx_uint16 size;   /* map's height/width in number of cells */
    struct yx_uint16 offset; /* the map scroll offset */
    char * cells;            /* sequence of bytes encoding map cells */
};



/* Initialize an island map as 64 x 64 cells of "~" cells representing water and
 * "." cells representing land. The shape of the island is generated randomly by
 * starting with a sea containing one land cell in the middle and then going
 * into a cycle of repeatedly selecting a random cell on the map and
 * transforming it into a land cell if it is horizontally or vertically neighbor
 * to one. The cycle ends when a land cell is created that is only one cell away
 * from the edge of the map. The map scroll offset is initialized to 0,0.
 */
extern struct Map init_map();

/* Scroll map into direction "dir" by changing the scroll offset if that does
 * not push the map view beyond the size of the map window as described by
 * "win_size".
 */
extern void map_scroll(struct Map * map, enum dir d, struct yx_uint16 win_size);

/* Scroll map to center on the player by changing the scroll offset following
 * (and constrained by) the window size as described by "win_size".
 */
extern void map_center_player(struct Map * map, struct Player * player,
                              struct yx_uint16 win_size);



#endif
