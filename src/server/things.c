/* src/server/things.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "things.h"
#include <stddef.h> /* NULL, size_t */
#include <stdint.h> /* uint8_t, uint16_t, int16_t, UINT8_MAX, UINT16_MAX */
#include <stdlib.h> /* free() */
#include <string.h> /* memset(), strcmp(), strdup(), strlen() */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* yx_uint8 */
#include "cleanup.h" /* set_cleanup_flag() */
#include "hardcoded_strings.h" /* s */
#include "field_of_view.h" /* build_fov_map() */
#include "map.h" /* mv_yx_in_dir_legal() */
#include "rrand.h" /* rrand() */
#include "thing_actions.h" /* actor_wait */
#include "world.h" /* world */



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
    for (; * nai_ptr_ptr; nai_ptr_ptr = &(*nai_ptr_ptr)->next);
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
    if (t->lifepoints && world.exists)
    {
        build_fov_map(t);
    }
    return t;
}



extern void add_thing_to_memory_map(struct Thing * t, uint8_t type,
                                    uint8_t y, uint8_t x)
{
    struct ThingInMemory * tm=try_malloc(sizeof(struct ThingInMemory),__func__);
    tm->type = type;
    tm->pos.y = y;
    tm->pos.x = x;
    tm->next = t->t_mem;
    t->t_mem = tm;
}



extern void free_thing_actions(struct ThingAction * ta)
{
    if (!ta)
    {
        return;
    }
    free_thing_actions(ta->next);
    free(ta->name);
    free(ta);
}



extern void free_thing_types(struct ThingType * tt)
{
    if (!tt)
    {
        return;
    }
    free_thing_types(tt->next);
    free(tt->name);
    free(tt);
}



extern void free_things(struct Thing * t)
{
    if (!t)
    {
        return;
    }
    free_things(t->owns);
    free_things(t->next);
    free(t->fov_map);
    free(t->mem_map);
    free_things_in_memory(t->t_mem);
    free(t);
    if (t == world.things)         /* So add_things()' NULL-delimited thing   */
    {                              /* iteration loop does not iterate over    */
        world.things = NULL;       /* freed memory when called the first time */
    }                              /* after world re-seeding.                 */
}



extern void free_things_in_memory(struct ThingInMemory * tm)
{
    if (!tm)
    {
        return;
    }
    free_things_in_memory(tm->next);
    free(tm);
}



extern struct ThingAction * get_thing_action(uint8_t id)
{
    struct ThingAction * ta = world.thing_actions;
    for (; ta && id != ta->id; ta = ta->next);
    return ta;
}



extern struct ThingType * get_thing_type(uint8_t id)
{
    struct ThingType * tt = world.thing_types;
    for (; tt && id != tt->id; tt = tt->next);
    return tt;
}



extern uint8_t get_thing_action_id_by_name(char * name)
{
    struct ThingAction * ta = world.thing_actions;
    while (ta)
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
        if (!ptr || id == ptr->id)
        {
            return ptr;
        }
        if (deep)
        {
            struct Thing * owned_thing = get_thing(ptr->owns, id, 1);
            if (owned_thing)
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



extern void try_thing_proliferation(struct Thing * t)
{
    struct ThingType * tt = get_thing_type(t->type);
    if (tt->proliferate)
    {
        if (1 == tt->proliferate || 1 == (rrand() % tt->proliferate))
        {
            struct yx_uint8 candidates[6];
            uint8_t n_candidates = 0;
            char dirs[7] = "dxswed";
            struct yx_uint8 start = t->pos;
            uint8_t i;
            for (i = 0; i < strlen(dirs); i++)
            {
                if (   mv_yx_in_dir_legal(dirs[i], &start)
                    && '.' == world.map.cells[start.y*world.map.length+start.x])
                {
                    uint8_t drop = 0;
                    if (tt->lifepoints)
                    {
                        for (t = world.things; t; t = t->next)
                        {
                            if (   t->lifepoints
                                && start.y == t->pos.y && start.x == t->pos.x)
                            {
                                drop = 1;
                            }
                        }
                    }
                    if (!drop)
                    {
                        candidates[n_candidates] = start;
                        n_candidates++;
                    }
                }
            }
            if (!n_candidates)
            {
                return;
            }
            i = rrand() % n_candidates;
            add_thing(-1, tt->id, candidates[i].y, candidates[i].x);
        }
    }
}



extern void add_things(uint8_t type, uint8_t n)
{
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        struct yx_uint8 pos;
        while (1)
        {
            char * err="Space to put thing on too hard to find. Map too small?";
            uint16_t i_pos = 0;
            for (pos.y = pos.x = 0;
                 '.' != world.map.cells[pos.y * world.map.length + pos.x];
                 i_pos++)
            {
                exit_err(UINT16_MAX == i_pos, err);
                pos.y = rrand() % world.map.length;
                pos.x = rrand() % world.map.length;
            }
            struct Thing * t;
            uint8_t clear = 1;
            for (t = world.things; t; t = t->next)
            {
                if (0 != t->lifepoints && pos.y==t->pos.y && pos.x==t->pos.x)
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
    for (; * t_ptr_ptr; t_ptr_ptr = &(*t_ptr_ptr)->next);
    * t_ptr_ptr = t;
    t->next = NULL;
}



extern void set_thing_position(struct Thing * t, struct yx_uint8 pos)
{
    t->pos = pos;
    struct Thing * owned = t->owns;
    for (; owned; set_thing_position(owned, pos), owned = owned->next);
}
