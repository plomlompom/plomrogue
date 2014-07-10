/* src/server/init.h
 *
 * Server, world and game state initialization.
 */

#ifndef INIT_H
#define INIT_H

#include <stdint.h> /* uint32_t */



/* Parses command line arguments -v and -s into server configuration. */
extern void obey_argv(int argc, char * argv[]);

/* Start server in file and out file, latter with server process test string. */
extern void setup_server_io();

/* Dissolves old game world if it exists, generates a new one from world.seed.
 * Unlinks any pre-existing record file.
 *
 * Thing (action) definitions read in from server config directory are not
 * affected. The map is populated accordingly. world.turn is set to 1, as is
 * world.do_update, so that io_round() is told to update the worldstate file.
 */
extern void remake_world();

/* Create a game world state, then enter play or replay mode.
 *
 * If replay mode is called for, try for the record file and follow its commands
 + up to the turn specified by the user, then enter manual replay. Otherwise,
 * start into play mode after having either recreated a game world state from
 * the savefile, or, if none exists, having created a new world with the
 * MAKE_WORLD command. Manual replay as well as play mode take place inside
 * io_loop().
 */
extern void run_game();



#endif
