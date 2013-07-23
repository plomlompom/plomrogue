/* map_objects.h
 *
 * Structs for objects on the map and their type definitions, and routines to
 * initialize these and load and save them from/to files.
 */

#ifndef MAP_OBJECTS_H
#define MAP_OBJECTS_H



#include <stdio.h> /* for FILE typedef */
#include "yx_uint16.h" /* for yx_uint16 coordinates */
struct World;



/* Player is non-standard: single and of a hard-coded type. */
struct Player
{
    struct yx_uint16 pos;
    unsigned char hitpoints;
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
    unsigned char hitpoints;
};



/* Structs for map object *type* definitions. Values common to all members of
 * a single monster or item type are harvested from these.
 */

struct MapObjDef
{
    struct MapObjDef * next;
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
    unsigned char hitpoints_start; /* Hitpoints each monster starts with. */
};



/* Initialize map object type definitions from file at path "filename". */
extern void init_map_object_defs(struct World * world, char * filename);



/* Build into memory starting at "start" chain of "n" map objects of type
 * "def_id", pass either "build_map_objects_itemdata" or
 * "build_map_objects_monsterdata" as "b_typedata"() to build data specific
 * to monsters or items (or more forms if they ever get invented).
 *
 * TODO: function should decide by itself what "b_typedata"() to call based
 * on monster-or-item info in MapObjDef struct or from a table mapping type
 * identifiers to these.
 */
extern void * build_map_objects(struct World * world, void * start, char def_id,
                                 unsigned char n, size_t size,
                                 void (*) (struct MapObjDef *, void *));
extern void build_map_objects_itemdata(struct MapObjDef * map_obj_def,
                                       void * start);
extern void build_map_objects_monsterdata(struct MapObjDef * map_obj_def,
                                          void * start);



/* Write to/read from file chain of map objects starting/to start in memory at
 * "start", use "w_typedata"()"/"r_typedata" for data specific to monsters
 * (pass "write_map_objects_monsterdata"/"read_map_objects_itemdata") or items
 * (currently they have no data specific only to them, so pass NULL). Use "size"
 * in read_map_objects() to pass the size of structs of the affected map object
 * type.
 *
 * TODO: the size of these structs should not need to be passed but instead be
 * available via the type id of the affected map object type. The TODO above
 * towards the function deciding its helper function by itself also applies.
 */
extern void write_map_objects(void * start, FILE * file,
                              void (* w_typedata) (void *, FILE *) );
extern void read_map_objects(void * start, FILE * file, size_t size,
                             void (* w_typedata) (void *, FILE *) );
extern void write_map_objects_monsterdata(void * start, FILE * file);
extern void read_map_objects_monsterdata( void * start, FILE * file);



/* Get pointer to the map object definition of identifier "def_id". */
extern struct MapObjDef * get_map_obj_def(struct World * world, char def_id);



#endif
