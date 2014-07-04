/* src/server/world.h
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
    uint16_t last_update_turn; /* Last turn the .path_out file was updated. */
    uint8_t player_type; /* Thing type that player will start as. */
    uint8_t is_verbose; /* Should server send debugging info to stdout? */
    uint8_t enemy_fov; /* != 0 if non-player actors only see field of view. */
};

extern struct World world;



#endif
