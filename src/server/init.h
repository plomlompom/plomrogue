/* src/server/init.h
 *
 * Server, world and game state initialization.
 */

#ifndef INIT_H
#define INIT_H

#include <stdint.h> /* uint32_t */



/* Parses command line arguments -v and -s into server configuration. */
extern void obey_argv(int argc, char * argv[]);

/* Dissolves old game world if it exists, and generates a new one from "seed".
 *
 * Map object (action) definitions read in from server config directory are not
 * affected. world.last_update_turn is set to 0 and world.turn to 1, so that
 * io_round()'s criteria for updating the output file are triggered even when
 * this function is called during a round 1. How many map objects of what type
 * id are generated on the map is currently hard-coded.
 */
extern void remake_world(uint32_t seed);

/* Create a game state from which to play or replay, then enter io_loop().
 *
 * If no record file exists at world.path_record, generate new world (by a
 * "seed" command calling remake_world()) in play mode, or error-exit in replay
 * mode. If a record file exists, in play mode auto-replay it up to the last
 * game state before turning over to the player; in replay mode, auto-replay it
 * up to the turn named in world.replay and then turn over to manual replay.
 */
extern void run_game();



#endif
