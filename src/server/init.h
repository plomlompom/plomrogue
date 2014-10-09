/* src/server/init.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Server, world and game state initialization.
 */

#ifndef INIT_H
#define INIT_H

#include <stdint.h> /* uint8_t */



/* Parses command line arguments -v and -s into server configuration. */
extern void obey_argv(int argc, char * argv[]);

/* Start server in file and out file, latter with server process test string. */
extern void setup_server_io();

/* Dissolves old game world if it exists, generates a new one from world.seed.
 * The map is populated according to world.thing_types start numbers. world.turn
 * is set to 1, as is .exists and .do_update, so that io_round() is told to
 * update the worldstate file. Returns 0 on success, and if the world cannot be
 * generated 1 since there is no player type or it has .n_start of 0, 2 if no
 * "wait" thing action is defined.
 */
extern uint8_t remake_world();

/* Create a game world state, then enter play or replay mode.
 *
 * If replay mode is called for, try for the record file and follow its commands
 + up to the turn specified by the user, then enter manual replay. Otherwise,
 * start into play mode after having either recreated a game world state from
 * the savefile, or, if none exists, having created a new world with first
 * following the commands from the world config file, then running the
 * MAKE_WORLD command. Manual replay as well as manual play mode take place
 * inside io_loop().
 */
extern void run_game();



#endif
