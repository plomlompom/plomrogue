/* map.h
 *
 * Struct for the game map and routines to manipulate it.
 */

#ifndef MAP_H
#define MAP_H

#include "yx_uint16.h" /* for yx_uint16 and dir enums */
struct Win;



struct Map
{
    struct yx_uint16 size;   /* map's height/width in number of cells */
    char * cells;            /* sequence of bytes encoding map cells */
};



/* Initialize an island map as 64 x 64 cells of "~" cells representing water and
 * "." cells representing land. The shape of the island is generated randomly by
 * starting with a sea containing one land cell in the middle and then going
 * into a cycle of repeatedly selecting a random cell on the map and
 * transforming it into a land cell if it is horizontally or vertically neighbor
 * to one; the cycle ends when a land cell is due to be created right at the
 * border of the map.
 */
extern struct Map init_map();



/* Try to change the view center of map "win" of "map_size" into dir "d". */
extern void map_scroll(struct Win * win, struct yx_uint16 map_size, enum dir d);



#endif
