/* src/client/map.h
 *
 * Routines for the game map window.
 */

#ifndef MAP_H_CLIENT
#define MAP_H_CLIENT



/* Try changing map window's focus into direction "d" (north = "N" etc.). */
extern void map_scroll(char d);

/* Center map window on player (even if it is non-visible). */
extern void map_center();



#endif
