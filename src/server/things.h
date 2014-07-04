/* src/server/things.h
 *
 * Structs for things and their type definitions, and routines to initialize
 * these and load and save them from/to files.
 */

#ifndef THINGS_H
#define THINGS_H

#include <stdint.h> /* uint8_t */
#include "../common/yx_uint8.h" /* yx_uint8 structs */



struct Thing
{
    struct Thing * next;         /* pointer to next one in things chain */
    struct Thing * owns;         /* chain of things owned / in inventory */
    struct yx_uint8 pos;         /* coordinate on map */
    uint8_t * fov_map;           /* map of the thing's field of view */
    uint8_t id;                  /* individual thing's unique identifier */
    uint8_t type;                /* ID of appropriate thing definition */
    uint8_t lifepoints;          /* 0: thing is inanimate; >0: hitpoints */
    uint8_t command;             /* thing's current action; 0 if none */
    uint8_t arg;                 /* optional field for .command argument */
    uint8_t progress;            /* turns already passed to realize .command */
};

struct ThingType
{
    uint8_t id;         /* thing type identifier / sets .type */
    struct ThingType * next;
    char char_on_map;   /* thing symbol to appear on map */
    char * name;        /* string to describe thing in game log */
    uint8_t corpse_id;  /* type to change thing into upon destruction */
    uint8_t lifepoints; /* default start value for thing's .lifepoints */
    uint8_t consumable; /* can be eaten if !0, for so much hitpoint win */
    uint8_t start_n;    /* how many of these does the map start with? */
};



/* Return thing of "id" in chain at "ptr", search inventories too if "deep". */
extern struct Thing * get_thing(struct Thing * ptr, uint8_t id, uint8_t deep);

/* Free thing types chain starting at "tt_start". */
extern void free_thing_types(struct ThingType * tt_start);

/* Add thing of "id" and "type" to map on random passable position (positions
 * which contain an actor are not deemed passable) if "find_pos", else on y=0,
 * x=0. If "id" is >= 0 and <= UINT8_MAX, use lowest unused id. Return thing.
 */
extern struct Thing * add_thing(int16_t id, uint8_t type, uint8_t find_pos);

/* Add thing(s) ("n": how many?) of "type" to map on random position(s). New
 * animate things are never placed in the same square with other animate ones.
 */
extern void add_things(uint8_t type, uint8_t n);

/* Free things in things chain starting at "t_start. */
extern void free_things(struct Thing * t_start);

/* Move thing of "id" from "source" inventory to "target" inventory. */
extern void own_thing(struct Thing ** target, struct Thing ** source,
                      uint8_t id);

/* Get pointer to the Thing struct that represents the player. */
extern struct Thing * get_player();

/* Get pointer to the thing type of identifier "def_id". */
extern struct ThingType * get_thing_type(uint8_t id);

/* Move not only "t" to "pos", but also all things owned by it. */
extern void set_thing_position(struct Thing * t, struct yx_uint8 pos);



#endif
