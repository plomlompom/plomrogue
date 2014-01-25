/* src/client/map_window.h
 *
 * Routines to re-focus the game map window.
 */

#ifndef MAP_WINDOW_H
#define MAP_WINDOW_H



/* Try changing map window's focus into direction "d" (north = "N" etc.). */
extern void map_scroll(char d);

/* Center map window on player (even if it is non-visible). */
extern void map_center();



#endif
