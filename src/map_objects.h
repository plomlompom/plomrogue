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



struct MapObj
{
    struct MapObj * next;        /* pointer to next one in map object chain */
    struct MapObj * owns;        /* chain of map objects owned / in inventory */
    uint8_t id;                  /* individual map object's unique identifier */
    uint8_t type;                /* ID of appropriate map object definition */
    uint8_t lifepoints;          /* 0: object is inanimate; >0: hitpoints */
    struct yx_uint16 pos;        /* coordinate on map */
};



struct MapObjDef
{
    struct MapObjDef * next;
    uint8_t id;         /* unique identifier of map object type */
    uint8_t corpse_id;  /* id of type to change into upon destruction */
    char char_on_map;   /* map object symbol to appear on map */
    char * name;        /* string to describe object in game log*/
    uint8_t lifepoints; /* default value for map object lifepoints member */
};



/* Initialize map object defnitions chain from file at path "filename". */
extern void init_map_object_defs(char * filename);



/* Free map object definitions chain starting at "mod_start". */
extern void free_map_object_defs(struct MapObjDef * mod_start);



/* Add new object(s) ("n": how many?) of "type" to map on random position(s).
 * New animate objects are never placed in the same square with other animated
 * ones.
 */
extern void add_map_object(uint8_t type);
extern void add_map_objects(uint8_t type, uint8_t n);



/* Write map objects chain to "file". */
extern void write_map_objects(FILE * file);

/* Read from "file" map objects chain; use "line" as char array for fgets() and
 * expect strings of max. "linemax" length.
 */
extern void read_map_objects(FILE * file, char * line, int linemax);



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
