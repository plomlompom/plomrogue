/* src/client/map.h
 *
 * Routines for the game map window.
 */

#ifndef MAP_H
#define MAP_H

#include "../common/yx_uint16.h" /* yx_uint16 struct */


struct Map
{
    struct yx_uint16 size;   /* map's height/width in number of cells */
    char * cells;            /* sequence of bytes encoding map cells */
};



/* Try changing map window's focus into direction "d" (north = "N" etc.). */
extern void map_scroll(char d);

/* Center map window on player (even if it is non-visible). */
extern void map_center();



#endif
