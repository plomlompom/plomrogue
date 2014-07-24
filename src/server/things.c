/* src/server/things.c */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "things.h"
#include <stddef.h> /* NULL, size_t */
#include <stdint.h> /* uint8_t, uint16_t, int16_t, UINT8_MAX, UINT16_MAX */
#include <stdlib.h> /* free() */
#include <string.h> /* memset(), strcmp(), strdup() */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* yx_uint8 */
#include "cleanup.h" /* set_cleanup_flag() */
#include "hardcoded_strings.h" /* s */
#include "map.h" /* is_passable() */
#include "rrand.h" /* rrand() */
#include "thing_actions.h" /* actor_wait */
#include "world.h" /* world */
#include "yx_uint8.h" /* yx_uint8_cmp() */



/* Used to treat structs Thing, ThingType and ThingAction the same. */
struct NextAndId
{
    struct NextAndId * next;
    uint8_t id;
};



/* To linked list of NextAndId structs (or rather structs whose start region is
 * compatible to it) starting at "start", add newly allocated element of
 * "n_size" and an ID that is either "id" or, if "id" is <= UINT8_MAX and >=
 * "id_start", get lowest ID >= "start_id" and <= UINT8_MAX for new thing
 * ("struct_id"=0), thing type ("struct_id"=1) or thing action ("struct_id"=2).
 */
static struct NextAndId * add_to_struct_list(size_t n_size, uint8_t start_id,
                                             int16_t id, uint8_t struct_id,
                                             struct NextAndId ** start);



static struct NextAndId * add_to_struct_list(size_t n_size, uint8_t start_id,
                                             int16_t id, uint8_t struct_id,
                                             struct NextAndId ** start)
{
    struct NextAndId * nai  = try_malloc(n_size, __func__);
    memset(nai, 0, n_size);
    if (start_id <= id && id <= UINT8_MAX)
    {
        nai->id = id;
    }
    else
    {
        while (1)
        {
            if (   (0 == struct_id && !get_thing(world.things, start_id, 1))
                || (1 == struct_id && !get_thing_type(start_id))
                || (2 == struct_id && !get_thing_action(start_id)))
            {
                nai->id = start_id;
                break;
            }
            char * err =  "No unused ID available to add to ID list.";
            exit_err(start_id == UINT8_MAX, err);
            start_id++;
        }
    }
    struct NextAndId ** nai_ptr_ptr = start;
    for (; NULL != * nai_ptr_ptr; nai_ptr_ptr = &(*nai_ptr_ptr)->next);
    *nai_ptr_ptr = nai;
    return nai;
}



extern struct ThingAction * add_thing_action(uint8_t id)
{
    struct ThingAction * ta;
    ta = (struct ThingAction *) add_to_struct_list(sizeof(struct ThingAction),
                                                   1, (int16_t) id, 2,
                                                   (struct NextAndId **)
                                                   &world.thing_actions);
    set_cleanup_flag(CLEANUP_THING_ACTIONS);
    ta->name = strdup(s[S_CMD_WAIT]);
    ta->effort = 1;
    ta->func = actor_wait;
    return ta;
}



extern struct ThingType * add_thing_type(int16_t id)
{
    struct ThingType * tt;
    tt = (struct ThingType *) add_to_struct_list(sizeof(struct ThingType),
                                                 0, id, 1,
                                                 (struct NextAndId **)
                                                 &world.thing_types);
    set_cleanup_flag(CLEANUP_THING_TYPES);
    tt->name = strdup("(none)");
    tt->corpse_id = tt->id;
    return tt;
}



extern struct Thing * add_thing(int16_t id, uint8_t type, uint8_t y, uint8_t x)
{
    struct Thing * t;
    t = (struct Thing *) add_to_struct_list(sizeof(struct Thing), 0, id, 0,
                                            (struct NextAndId **)&world.things);
    struct ThingType * tt = get_thing_type(type);
    set_cleanup_flag(CLEANUP_THINGS);
    t->type       = tt->id;
    t->lifepoints = tt->lifepoints;
    t->pos.y      = y;
    t->pos.x      = x;
    return t;
}



extern void free_thing_actions(struct ThingAction * ta)
{
    if (NULL == ta)
    {
        return;
    }
    free_thing_actions(ta->next);
    free(ta->name);
    free(ta);
}



extern void free_thing_types(struct ThingType * tt)
{
    if (NULL == tt)
    {
        return;
    }
    free_thing_types(tt->next);
    free(tt->name);
    free(tt);
}



extern void free_things(struct Thing * t)
{
    if (NULL == t)
    {
        return;
    }
    free_things(t->owns);
    free_things(t->next);
    free(t->fov_map);
    free(t);
    if (t == world.things)         /* So add_things()' NULL-delimited thing   */
    {                              /* iteration loop does not iterate over    */
        world.things = NULL;       /* freed memory when called the first time */
    }                              /* after world re-seeding.                 */
}



extern struct ThingAction * get_thing_action(uint8_t id)
{
    struct ThingAction * ta = world.thing_actions;
    for (; NULL != ta && id != ta->id; ta = ta->next);
    return ta;
}



extern struct ThingType * get_thing_type(uint8_t id)
{
    struct ThingType * tt = world.thing_types;
    for (; NULL != tt && id != tt->id; tt = tt->next);
    return tt;
}



extern uint8_t get_thing_action_id_by_name(char * name)
{
    struct ThingAction * ta = world.thing_actions;
    while (NULL != ta)
    {
        if (0 == strcmp(ta->name, name))
        {
            break;
        }
        ta = ta->next;
    }
    if (!ta)
    {
        return 0;
    }
    return ta->id;
}



extern struct Thing * get_thing(struct Thing * ptr, uint8_t id, uint8_t deep)
{
    while (1)
    {
        if (NULL == ptr || id == ptr->id)
        {
            return ptr;
        }
        if (deep)
        {
            struct Thing * owned_thing = get_thing(ptr->owns, id, 1);
            if (NULL != owned_thing)
            {
                return ptr;
            }
        }
        ptr = ptr->next;
    }
}



extern struct Thing * get_player()
{
    return get_thing(world.things, 0, 0);
}



extern void add_things(uint8_t type, uint8_t n)
{
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        struct yx_uint8 pos;
        while (1)
        {
            char * err = "Space to put thing on too hard to find."
                         "Map too small?";
            uint16_t i_pos = 0;
            for (pos.y = pos.x = 0; 0 == is_passable(pos); i_pos++)
            {
                exit_err(UINT16_MAX == i_pos, err);
                pos.y = rrand() % world.map.length;
                pos.x = rrand() % world.map.length;
            }
            struct Thing * t;
            uint8_t clear = 1;
            for (t = world.things; t; t = t->next)
            {
                if (yx_uint8_cmp(&pos, &t->pos) && 0 != t->lifepoints)
                {
                    clear = 0;
                    break;
                }
            }
            if (1 == clear)
            {
                break;
            }
        }
        add_thing(-1, type, pos.y, pos.x);
    }
}



extern void own_thing(struct Thing ** target, struct Thing ** source,
                      uint8_t id)
{
    struct Thing * t;
    if (id == (*source)->id)
    {
        t = * source;
        * source = t->next;
    }
    else
    {
        struct Thing * penult = * source;
        while (1)
        {
            if (id == penult->next->id)
            {
                break;
            }
            penult = penult->next;
        }
        t = penult->next;
        penult->next = t->next;
    }
    struct Thing ** t_ptr_ptr = target;
    for (; NULL != * t_ptr_ptr; t_ptr_ptr = &(*t_ptr_ptr)->next);
    * t_ptr_ptr = t;
    t->next = NULL;
}



extern void set_thing_position(struct Thing * t, struct yx_uint8 pos)
{
    t->pos = pos;
    struct Thing * owned = t->owns;
    for (; owned != NULL; set_thing_position(owned, pos), owned = owned->next);
}
