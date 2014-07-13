/* src/server/things.h
 *
 * Structs for things and their type and action definitions, and routines to
 * initialize these.
 */

#ifndef THINGS_H
#define THINGS_H

#include <stdint.h> /* uint8_t, int16_t */
#include "../common/yx_uint8.h" /* yx_uint8 structs */



struct Thing
{
    struct Thing * next;
    uint8_t id;                  /* individual thing's unique identifier */
    struct Thing * owns;         /* chain of things owned / in inventory */
    struct yx_uint8 pos;         /* coordinate on map */
    uint8_t * fov_map;           /* map of the thing's field of view */
    uint8_t type;                /* ID of appropriate thing definition */
    uint8_t lifepoints;          /* 0: thing is inanimate; >0: hitpoints */
    uint8_t command;             /* thing's current action; 0 if none */
    uint8_t arg;                 /* optional field for .command argument */
    uint8_t progress;            /* turns already passed to realize .command */
};

struct ThingType
{
    struct ThingType * next;
    uint8_t id;         /* thing type identifier / sets .type */
    char char_on_map;   /* thing symbol to appear on map */
    char * name;        /* string to describe thing in game log */
    uint8_t corpse_id;  /* type to change thing into upon destruction */
    uint8_t lifepoints; /* default start value for thing's .lifepoints */
    uint8_t consumable; /* can be eaten if !0, for so much hitpoint win */
    uint8_t start_n;    /* how many of these does the map start with? */
};

struct ThingAction
{
    struct ThingAction * next;
    uint8_t id;   /* identifies action in Thing.command; therefore must be >0 */
    void (* func) (struct Thing *);    /* function called after .effort turns */
    char * name;                       /* human-readable identifier */
    uint8_t effort;                    /* how many turns the action takes */
};



/* Add thing action of "id" to world.thing_actions, with .name defaulting to
 * s[S_CMD_WAIT], .func to actor_wait() and .effort to 1. If "id" is not >= 1
 * and <= UINT8_MAX, use lowest unused id. Return thing action.
 */
extern struct ThingAction * add_thing_action(int16_t id);

/* Add thing type of "id", with .corpse_id defaulting to "id" to
 * world.thing_types, .name to "(none)" and the remaining values to 0. If "id"
 * is not >= 0 and <= UINT8_MAX, use lowest unused id. Return thing type.
 */
extern struct ThingType * add_thing_type(int16_t id);

/* Add thing of "id" and "type" on position of "y"/x" to world.things.If "id" is
 * not >= 0 and <= UINT8_MAX, use lowest unused id. Return thing.
 */
extern struct Thing * add_thing(int16_t id, uint8_t type, uint8_t y, uint8_t x);

/* Free ThingAction/ThingType/Thing * chain starting at "ta"/"tt"/"t". */
extern void free_thing_actions(struct ThingAction * ta);
extern void free_thing_types(struct ThingType * tt);
extern void free_things(struct Thing * t);

/* Return pointer to ThingAction/ThingType of "id", or NULL if none found. */
extern struct ThingAction * get_thing_action(uint8_t id);
extern struct ThingType * get_thing_type(uint8_t id);

/* Return world.thing_actions ThingAction.id for "name" or 0 if none found. */
extern uint8_t get_thing_action_id_by_name(char * name);

/* Return thing of "id" in chain at "ptr", search inventories too if "deep".
 * Return NULL if nothing found.
 */
extern struct Thing * get_thing(struct Thing * ptr, uint8_t id, uint8_t deep);

/* Get pointer to the non-owend Thing struct that represents the player, or NULL
 * if none found.
 */
extern struct Thing * get_player();

/* Add thing(s) ("n": how many?) of "type" to map on random passable
 * position(s). New animate things are never placed in the same square with
 * other animate ones.
 */
extern void add_things(uint8_t type, uint8_t n);

/* Move thing of "id" from "source" inventory to "target" inventory. */
extern void own_thing(struct Thing ** target, struct Thing ** source,
                      uint8_t id);

/* Move not only "t" to "pos", but also all things owned by it. */
extern void set_thing_position(struct Thing * t, struct yx_uint8 pos);



#endif
