/* src/server/world.h
 *
 * Contains the World struct holding all game data together.
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include "map.h" /* struct Map */
struct MapObjDef;
struct MapObjAct;
struct MapObj;



struct World
{
    struct Map map;
    struct MapObjDef * map_obj_defs; /* Map object definitions. */
    struct MapObjAct * map_obj_acts; /* Map object action definitions. */
    struct MapObj * map_objs; /* Map objects. */
    char * log; /* Logs the game events from the player's view. */
    char * path_in; /* Fifo to receive command messages. */
    char * path_out; /* File to write the game state as visible to clients.*/
    char * path_record; /* Record file from which to read the game history. */
    char * path_map_obj_defs; /* path for map object definitions config file */
    char * path_map_obj_acts; /* path for map object actions config file */
    char * tmp_suffix; /* Appended to paths of files for their tmp versions. */
    char * queue; /* Stores un-processed messages received via input fifo. */
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
