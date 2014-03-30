/* src/server/world.h
 *
 * Contains the World struct holding all game data together.
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* define FILE */
#include "map.h" /* struct Map */
struct MapObjDef;
struct MapObjAct;
struct MapObj;



struct World
{
    FILE * file_in; /* Input stream on file at .path_in. */
    FILE * file_out; /* Output stream on file at .path_out. */
    struct Map map;
    struct MapObjDef * map_obj_defs; /* Map object definitions. */
    struct MapObjAct * map_obj_acts; /* Map object action definitions. */
    struct MapObj * map_objs; /* Map objects. */
    char * log; /* Logs the game events from the player's view. */
    char * server_test; /* String uniquely identifying server process. */
    char * path_in; /* File to write client messages into. */
    char * path_out; /* File to write server messages into. */
    char * path_worldstate; /* File to represent world state  to clients.*/
    char * path_record; /* Record file from which to read the game history. */
    char * path_config; /* Path for map object (action) definitions file. */
    char * tmp_suffix; /* Appended to paths of files for their tmp versions. */
    char * queue; /* Stores un-processed messages read from the input file. */
    uint32_t queue_size;/* Length of .queue sequence of \0-terminated strings.*/
    uint32_t seed; /* Randomness seed. */
    uint16_t replay; /* Turn up to which to replay game. No replay if zero. */
    uint16_t turn; /* Current game turn. */
    uint16_t last_update_turn; /* Last turn the .path_out file was updated. */
    uint8_t is_verbose; /* Should server send debugging info to stdout? */
    uint8_t map_obj_count; /* Counts map objects generated so far. */
};

extern struct World world;



#endif
