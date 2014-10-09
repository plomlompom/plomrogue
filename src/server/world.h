/* src/server/world.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Contains the World struct holding all game data together.
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* define FILE */
#include "../common/map.h" /* struct Map */
struct ThingType;
struct ThingAction;
struct Thing;



struct World
{
    FILE * file_in; /* Input stream on file at .path_in. */
    FILE * file_out; /* Output stream on file at .path_out. */
    struct Map map; /* Game map. */
    struct ThingType * thing_types; /* Thing type definitions. */
    struct ThingAction * thing_actions; /* Thing action definitions. */
    struct Thing * things; /* All physical things of the game world. */
    char * log; /* Logs the game events from the player's view. */
    char * server_test; /* String uniquely identifying server process. */
    char * queue; /* Stores un-processed messages read from the input file. */
    uint32_t queue_size;/* Length of .queue sequence of \0-terminated strings.*/
    uint32_t seed; /* Randomness seed. */
    uint32_t seed_map; /* Map seed. */
    uint16_t replay; /* Turn up to which to replay game. No replay if zero. */
    uint16_t turn; /* Current game turn. */
    uint8_t do_update; /* Update worldstate file if !0. */
    uint8_t exists; /* If !0, remake_world() has been run successfully. */
    uint8_t player_type; /* Thing type that player will start as. */
    uint8_t is_verbose; /* Should server send debugging info to stdout? */
};

extern struct World world;



#endif
