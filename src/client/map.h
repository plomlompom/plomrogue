/* src/client/map.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Routines for the game map window.
 */

#ifndef MAP_H_CLIENT
#define MAP_H_CLIENT

#include <stdint.h> /* uint8_t */


/* Try changing map window's focus into direction "d" (north = "N" etc.). Unset
 * world.focus_each_turn.
 */
extern void map_scroll(char d);

/* Center map window on player (even if it is non-visible). */
extern void map_center();

/* Toggle world.focus_each_turn (auto-centering of map on player each turn). */
extern void toggle_autofocus();

/* Toggle world.look (moving look cursor instead of player over map). */
extern void toggle_lookmode();

/* Read "command" as look cursor move command, act on it.*/
extern uint8_t lookmode_nav(char * command);

/* Send THINGS_HERE query message to server.*/
extern void query_mapcell();



#endif
