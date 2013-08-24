/* map_objects.h
 *
 * Structs for objects on the map and their type definitions, and routines to
 * initialize these and load and save them from/to files.
 */

#ifndef MAP_OBJECTS_H
#define MAP_OBJECTS_H



#include <stdio.h> /* for FILE typedef */
#include <stdint.h> /* for uint8_t */
#include "yx_uint16.h" /* for yx_uint16 coordinates */
struct World;



/* Player is non-standard: single and of a hard-coded type. */
struct Player
{
    struct yx_uint16 pos;
    uint8_t hitpoints;
};



/* Structs for standard map objects. */

struct MapObj
{
    void * next;
    char type;            /* Map object type identifier (see MapObjDef.id). */
    struct yx_uint16 pos; /* Coordinate of object on map. */
};

struct Item
{
    struct MapObj map_obj;
};

struct Monster
{
    struct MapObj map_obj;
    uint8_t hitpoints;
};



/* Structs for map object *type* definitions. Values common to all members of
 * a single monster or item type are harvested from these.
 */

struct MapObjDef
{
    struct MapObjDef * next;
    char m_or_i;  /* Is it item or monster? "i" for items, "m" for monsters. */
    char id;      /* Unique identifier of the map object type to describe. */
    char mapchar; /* Map object symbol to appear on map.*/
    char * desc;  /* String describing map object in the game log. */
};

struct ItemDef
{
    struct MapObjDef map_obj_def;
};

struct MonsterDef
{
    struct MapObjDef map_obj_def;
    uint8_t hitpoints_start; /* Hitpoints each monster starts with. */
};



/* Initialize map object type definitions from file at path "filename". */
extern void init_map_object_defs(struct World * world, char * filename);



/* Build into memory starting at "start" chain of "n" map objects of type
 * "def_id".
 */
extern void * build_map_objects(struct World * world, void * start, char def_id,
                                uint8_t n);



/* Write to/read from file chain of map objects starting/to start in memory at
 * "start".
 */
extern uint8_t write_map_objects(struct World * world, void * start,
                                 FILE * file);
extern uint8_t read_map_objects(struct World * world, void * start,
                                FILE * file);



/* Get pointer to the map object definition of identifier "def_id". */
extern struct MapObjDef * get_map_obj_def(struct World * world, char def_id);



#endif
