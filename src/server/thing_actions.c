/* src/server/thing_actions.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "thing_actions.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), memcpy(), strncmp() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* yx_uint8 */
#include "field_of_view.h" /* build_fov_map() */
#include "hardcoded_strings.h" /* s */
#include "things.h" /* Thing, ThingType, get_player(), own_thing(),
                     * set_thing_position(), get_thing_type(),
                     * free_things_in_memory()
                     */
#include "map.h" /* mv_yx_in_dir_legal() */
#include "world.h" /* global world */



/* How many previous characters of the game log to keep on adding new text */
#define MAX_BACKLOG_CHARS 3000



/* If "text" is equal "log"'s last line, return 1, else 0. */
static uint8_t text_equals_log_end(char * log, char * text);

/* Append "text" to game log shortened to MAX_BACKLOG_CHARS characters, or
 * continuation period if "text" is the same as the (shortened) log's last line
 * minus continuation periods.
 */
static void update_log(char * text);

/* One actor "wounds" another actor, decrementing his lifepoints and, if they
 * reach zero in the process, killing it. Generates appropriate log message.
 */
static void actor_hits_actor(struct Thing * hitter, struct Thing * hitted);

/* Bonus stuff to actor_*() to happen if actor==player. Mostly writing of log
 * messages; _pick and _drop also decrement world.inventory_sel by 1 if >0.
 * (match_dir() is just a little helper to playerbonus_move().)
 */
static void playerbonus_wait();
static uint8_t match_dir(char d, char ** dsc_d, char match, char * dsc_match);
static void playerbonus_move(char d, uint8_t passable);
static void playerbonus_drop(uint8_t owns_none);
static void playerbonus_pick(uint8_t picked);
static void playerbonus_use(uint8_t no_thing, uint8_t wrong_thing);



static uint8_t text_equals_log_end(char * log, char * text)
{
    uint16_t len_old = strlen(log);
    uint16_t last_nl = len_old - 1;
    while (last_nl != 0)
    {
        if ('\n' == log[last_nl])
        {
            break;
        }
        last_nl--;
    }
    uint16_t last_stop = len_old - 1;
    while (last_stop != 0)
    {
        if ('.' == log[last_stop] && '.' != log[last_stop - 1])
        {
            break;
        }
        last_stop--;
    }
    if (   (last_stop + 1) - last_nl == (uint16_t) strlen(text)
        && 0 == strncmp(log + last_nl, text, strlen(text)))
    {
        return 1;
    }
    return 0;
}



static void update_log(char * text)
{
    uint16_t len_new = strlen(text);
    uint16_t len_old = 0;
    uint16_t offset = 0;
    if (world.log)
    {
        len_old = strlen(world.log);
        if (len_old > MAX_BACKLOG_CHARS)
        {
            offset = len_old - MAX_BACKLOG_CHARS;
            len_old = MAX_BACKLOG_CHARS;
        }
        if (text_equals_log_end(world.log + offset, text))
        {
            text = ".";
        }
    }
    uint16_t len_whole = len_old + len_new + 1;
    char * new_text = try_malloc(len_whole, __func__);
    memcpy(new_text, world.log + offset, len_old);
    int test = sprintf(new_text + len_old, "%s", text);
    exit_trouble(test < 0, __func__, s[S_FCN_SPRINTF]);
    free(world.log);
    world.log = new_text;
}



static void actor_hits_actor(struct Thing * hitter, struct Thing * hitted)
{
    struct ThingType * tt_hitter = get_thing_type(hitter->type);
    struct ThingType * tt_hitted = get_thing_type(hitted->type);
    struct Thing * player = get_player();
    char * msg1 = "You";
    char * msg2 = "wound";
    char * msg3 = "you";
    if      (player != hitter)
    {
        msg1 = tt_hitter->name;
        msg2 = "wounds";
    }
    if (player != hitted)
    {
        msg3 = tt_hitted->name;
    }
    uint8_t len = 1 + strlen(msg1) + 1 + strlen(msg2) + 1 + strlen(msg3) + 2;
    char * msg = try_malloc(len, __func__);
    int test = sprintf(msg, "\n%s %s %s.", msg1, msg2, msg3);
    exit_trouble(test < 0, __func__, s[S_FCN_SPRINTF]);
    update_log(msg);
    free(msg);
    hitted->lifepoints--;
    if (0 == hitted->lifepoints)
    {
        hitted->type = tt_hitted->corpse_id;
        if (player == hitted)
        {
            update_log(" You die.");
            memset(hitted->fov_map, ' ', world.map.length * world.map.length);
            return;
        }
        else
        {
            free(hitted->fov_map);
            hitted->fov_map = NULL;
            free(hitted->mem_map);
            hitted->mem_map = NULL;
            free_things_in_memory(hitted->t_mem);
            hitted->t_mem = NULL;
        }
        update_log(" It dies.");
    }
}



static void playerbonus_wait()
{
        update_log("\nYou wait.");
}



static uint8_t match_dir(char d, char ** dsc_d, char match, char * dsc_match)
{
    if (d == match)
    {
        * dsc_d = dsc_match;
        return 1;
    }
    return 0;
}



static void playerbonus_move(char d, uint8_t passable)
{
    char * dsc_dir = "north-east";
    if (   match_dir(d, &dsc_dir, 'd', "east")
        || match_dir(d, &dsc_dir, 'c', "south-east")
        || match_dir(d, &dsc_dir, 'x', "south-west")
        || match_dir(d, &dsc_dir, 's', "west")
        || match_dir(d, &dsc_dir, 'w', "north-west"))
    {
        ;
    }
    char * dsc_move = "You move ";
    if (0 == passable)
    {
        dsc_move = "You fail to move ";
    }
    char * msg = try_malloc(strlen(dsc_move) + strlen (dsc_dir) + 3, __func__);
    int test = sprintf(msg, "\n%s%s.", dsc_move, dsc_dir);
    exit_trouble(test < 0, __func__, s[S_FCN_SPRINTF]);
    update_log(msg);
    free(msg);
}



static void playerbonus_drop(uint8_t owns_none)
{
    if (0 != owns_none)
    {
        update_log("\nYou try to drop an object, but you own none.");
        return;
    }
    update_log("\nYou drop an object.");
}



static void playerbonus_pick(uint8_t picked)
{
    if (picked)
    {
        update_log("\nYou pick up an object.");
        return;
    }
    update_log("\nYou try to pick up an object, but there is none.");
}



static void playerbonus_use(uint8_t no_thing, uint8_t wrong_thing)
{
    if      (no_thing)
    {
        update_log("\nYou try to use an object, but you own none.");
        return;
    }
    else if (wrong_thing)
    {
        update_log("\nYou try to use this object, but fail.");
        return;
    }
    update_log("\nYou consume MAGIC MEAT.");
}



extern void actor_wait(struct Thing * t)
{
    if (t == get_player())
    {
        playerbonus_wait();
    }
}



extern void actor_move(struct Thing * t)
{
    char d = t->arg;
    struct Thing * other_t;
    struct yx_uint8 target = t->pos;
    uint8_t legal_move = mv_yx_in_dir_legal(d, &target);
    mv_yx_in_dir_legal(0, NULL);
    uint8_t passable = 0;
    if (legal_move)
    {
        passable = '.' == world.map.cells[target.y*world.map.length + target.x];
        for (other_t = world.things; other_t != 0; other_t = other_t->next)
        {
            if (0 == other_t->lifepoints || other_t == t)
            {
                continue;
            }
            if (target.y == other_t->pos.y && target.x == other_t->pos.x)
            {
               actor_hits_actor(t, other_t);
               return;
            }
        }
    }
    if (passable)
    {
        set_thing_position(t, target);
        build_fov_map(t);
    }
    if (t == get_player())
    {
        playerbonus_move(d, passable);
    }
}



extern void actor_drop(struct Thing * t)
{
    uint8_t owns_none = (!t->owns);
    if (!owns_none)
    {
        uint8_t select = t->arg;
        struct Thing * owned = t->owns;
        uint8_t i = 0;
        for (; i != select; i++, owned = owned->next);
        own_thing(&world.things, &t->owns, owned->id);
    }
    if (t == get_player())
    {
        playerbonus_drop(owns_none);
    }
}



extern void actor_pick(struct Thing * t)
{
    struct Thing * picked = NULL;
    struct Thing * t_i;
    for (t_i = world.things; t_i; t_i = t_i->next)
    {
        if (t_i != t && t_i->pos.y == t->pos.y && t_i->pos.x == t->pos.x)
        {
            picked = t_i;
        }
    }
    if (picked)
    {
        own_thing(&t->owns, &world.things, picked->id);
        set_thing_position(picked, t->pos);
    }
    if (t == get_player())
    {
        playerbonus_pick(!(!picked));
    }
}



extern void actor_use(struct Thing * t)
{
    uint8_t wrong_thing = 1;
    uint8_t no_thing = (!t->owns);
    if (!no_thing)
    {
        uint8_t select = t->arg;
        uint8_t i = 0;
        struct Thing * selected = t->owns;
        for (; i != select; i++, selected = selected->next);
        struct ThingType * tt = get_thing_type(selected->type);
        if (tt->consumable)
        {
            wrong_thing = 0;
            struct Thing * next = selected->next;
            free(selected);
            if (0 < select)
            {
                select--;
                selected = t->owns;
                for (i = 0; i != select; i++, selected = selected->next);
                selected->next = next;
            }
            else
            {
                t->owns = next;
            }
            t->lifepoints = t->lifepoints + tt->consumable;
        }
    }
    if (t == get_player())
    {
        playerbonus_use(no_thing, wrong_thing);
    }
}
