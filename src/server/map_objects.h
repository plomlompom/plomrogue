/* src/server/map_objects.h
 *
 * Structs for objects on the map and their type definitions, and routines to
 * initialize these and load and save them from/to files.
 */

#ifndef MAP_OBJECTS_H
#define MAP_OBJECTS_H

#include <stdint.h> /* uint8_t */
#include "../common/yx_uint16.h" /* yx_uint16 structs */



struct MapObj
{
    struct MapObj * next;        /* pointer to next one in map object chain */
    struct MapObj * owns;        /* chain of map objects owned / in inventory */
    struct yx_uint16 pos;        /* coordinate on map */
    uint8_t id;                  /* individual map object's unique identifier */
    uint8_t type;                /* ID of appropriate map object definition */
    uint8_t lifepoints;          /* 0: object is inanimate; >0: hitpoints */
    uint8_t command;             /* map object's current action */
    uint8_t arg;                 /* optional field for .command argument */
    uint8_t progress;            /* turns already passed to realize .command */
};

struct MapObjDef
{
    struct MapObjDef * next;
    char char_on_map;   /* map object symbol to appear on map */
    char * name;        /* string to describe object in game log */
    uint8_t id;         /* map object definition identifier / sets .type */
    uint8_t corpse_id;  /* type to change map object into upon destruction */
    uint8_t lifepoints; /* default start value for map object's .lifepoints */
};



/* Initialize map object definitions chain. */
extern void init_map_object_defs();

/* Free map object definitions chain starting at "mod_start". */
extern void free_map_object_defs(struct MapObjDef * mod_start);

/* Add object(s) ("n": how many?) of "type" to map on random position(s). New
 * animate objects are never placed in the same square with other animate ones.
 */
extern void add_map_objects(uint8_t type, uint8_t n);

/* Free map objects in map object chain starting at "mo_start. */
extern void free_map_objects(struct MapObj * mo_start);

/* Move object of "id" from "source" inventory to "target" inventory. */
extern void own_map_object(struct MapObj ** target, struct MapObj ** source,
                           uint8_t id);

/* Get pointer to the MapObj struct that represents the player. */
extern struct MapObj * get_player();

/* Get pointer to the map object definition of identifier "def_id". */
extern struct MapObjDef * get_map_object_def(uint8_t id);

/* Move not only "mo" to "pos", but also all map objects owned by it. */
extern void set_object_position(struct MapObj * mo, struct yx_uint16 pos);



#endif
