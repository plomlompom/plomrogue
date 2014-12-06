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


/* Toggle world.look (moving look cursor instead of player over map). */
extern void toggle_lookmode();

/* Read "command" as look cursor move command, act on it.*/
extern uint8_t lookmode_nav(char * command);

/* Send THINGS_HERE query message to server.*/
extern void query_mapcell();



#endif
