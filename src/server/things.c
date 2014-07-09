/* src/server/things.c */

#include "things.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, UINT8_MAX, UINT16_MAX */
#include <stdlib.h> /* free() */
#include <string.h> /* memset(), strlen() */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* yx_uint8 */
#include "map.h" /* is_passable() */
#include "rrand.h" /* rrand() */
#include "world.h" /* global world */
#include "yx_uint8.h" /* yx_uint8_cmp() */




/* Return lowest unused id for new thing. */
static uint8_t get_lowest_unused_id();



static uint8_t get_lowest_unused_id()
{
    uint8_t i = 0;
    while (1)
    {
        if (!get_thing(world.things, i, 1))
        {
            return i;
        }
        exit_err(i == UINT8_MAX, "No unused ID available to add new thing.");
        i++;
    }
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



extern void free_thing_types(struct ThingType * tt_start)
{
    if (NULL == tt_start)
    {
        return;
    }
    free_thing_types(tt_start->next);
    free(tt_start->name);
    free(tt_start);
}



extern struct Thing * add_thing(int16_t id, uint8_t type, uint8_t find_pos)
{
    char * f_name = "add_thing()";
    struct ThingType * tt = get_thing_type(type);
    struct Thing *     t  = try_malloc(sizeof(struct Thing), f_name);
    memset(t, 0, sizeof(struct Thing));
    t->id = (0 <= id && id <= UINT8_MAX) ? id : get_lowest_unused_id();
    t->type       = tt->id;
    t->lifepoints = tt->lifepoints;
    char * err = "Space to put thing on too hard to find. Map too small?";
    uint16_t i = 0;
    memset(&(t->pos), 0, sizeof(struct yx_uint8));
    while (find_pos)
    {
        struct yx_uint8 pos;
        for (pos.y = pos.x = 0; 0 == is_passable(pos); i++)
        {
            exit_err(UINT16_MAX == i, err);
            pos.y = rrand() % world.map.length;
            pos.x = rrand() % world.map.length;
        }
        struct Thing * t_ptr;
        uint8_t clear = 1;
        for (t_ptr = world.things; t_ptr != NULL; t_ptr = t_ptr->next)
        {
            if (yx_uint8_cmp(&pos, &t_ptr->pos) && 0 != t_ptr->lifepoints)
            {
                clear = 0;
                break;
            }
        }
        if (1 == clear)
        {
            t->pos = pos;
            break;
        }
    }
    struct Thing ** t_ptr_ptr = &world.things;
    for (; NULL != * t_ptr_ptr; t_ptr_ptr = &(*t_ptr_ptr)->next);
    * t_ptr_ptr = t;
    return t;
}



extern void add_things(uint8_t type, uint8_t n)
{
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        add_thing(-1, type, 1);
    }
}



extern void free_things(struct Thing * t_start)
{
    if (NULL == t_start)
    {
        return;
    }
    free_things(t_start->owns);
    free_things(t_start->next);
    free(t_start->fov_map);
    free(t_start);
    if (t_start == world.things)   /* So add_things()' NULL-delimited thing   */
    {                              /* iteration loop does not iterate over    */
        world.things = NULL;       /* freed memory when called the first time */
    }                              /* after world re-seeding.                 */
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



extern struct Thing * get_player()
{
    return get_thing(world.things, 0, 1);
}



extern struct ThingType * get_thing_type(uint8_t id)
{
    char * f_name = "get_thing_type()";
    struct ThingType * tt = world.thing_types;
    for (; NULL != tt && id != tt->id; tt = tt->next);
    char * err_intro = "Requested thing type of unused ID ";
    uint16_t size = strlen(err_intro) + 3 + 1 + 1;
    char * err = try_malloc(size, f_name);
    exit_trouble(sprintf(err, "%s%d.", err_intro, id) < 0, f_name, "sprintf()");
    exit_err(NULL == tt, err);
    free(err);
    return tt;
}



extern void set_thing_position(struct Thing * t, struct yx_uint8 pos)
{
    t->pos = pos;
    struct Thing * owned = t->owns;
    for (; owned != NULL; set_thing_position(owned, pos), owned = owned->next);
}
