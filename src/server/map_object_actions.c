/* src/server/map_object_actions.c */

#include "map_object_actions.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* strlen(), strcmp(), memcpy(), strtok(), strncmp() */
#include "../common/readwrite.h" /* textfile_sizes(), try_fopen(), try_fclose(),
                                  * try_fgets()
                                  */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint16.h" /* yx_uint16 struct */
#include "cleanup.h" /* set_cleanup_flag() */
#include "map_objects.h" /* structs MapObj, MapObjDef, get_player(),
                          * set_object_position(), own_map_object(),
                          * get_map_object_def()
                          */
#include "map.h" /* is_passable() */
#include "yx_uint16.h" /* mv_yx_in_dir(), yx_uint16_cmp() */
#include "world.h" /* global world */



/* Append "text" to game log, or a "." if "text" is the same as the last one. */
static void update_log(char * text);

/* If "name" fits "moa"->name, set "moa"->func to "func". */
static uint8_t try_func_name(struct MapObjAct * moa,
                             char * name, void (* func) (struct MapObj *));

/* One actor "wounds" another actor, decrementing his lifepoints and, if they
 * reach zero in the process, killing it. Generates appropriate log message.
 */
static void actor_hits_actor(struct MapObj * hitter, struct MapObj * hitted);

/* Bonus stuff to actor_*() to happen if actor==player. Mostly writing of log
 * messages; _pick and _drop also decrement world.inventory_sel by 1 if >0.
 */
static void playerbonus_wait();
static void playerbonus_move(char d, uint8_t passable);
static void playerbonus_drop(uint8_t owns_none);
static void playerbonus_pick(uint8_t picked);
static void playerbonus_use(uint8_t no_object, uint8_t wrong_object);



static void update_log(char * text)
{
    char * f_name = "update_log()";
    uint16_t len_new = strlen(text);
    uint16_t len_old = 0;
    if (world.log)
    {
        len_old = strlen(world.log);
        uint16_t last_nl = len_old - 1;
        while (last_nl != 0)
        {
            if ('\n' == world.log[last_nl])
            {
                break;
            }
            last_nl--;
        }
        uint16_t last_stop = len_old - 1;
        while (last_stop != 0)
        {
            if ('.' == world.log[last_stop] && '.' != world.log[last_stop - 1])
            {
                break;
            }
            last_stop--;
        }
        if (   (last_stop + 1) - last_nl == strlen(text)
            && 0 == strncmp(world.log + last_nl, text, strlen(text)))
        {
            text = ".";
        }
    }
    uint16_t len_whole = len_old + len_new + 1;
    char * new_text = try_malloc(len_whole, f_name);
    memcpy(new_text, world.log, len_old);
    sprintf(new_text + len_old, "%s", text);
    free(world.log);
    world.log = new_text;
}



static uint8_t try_func_name(struct MapObjAct * moa,
                             char * name, void (* func) (struct MapObj *))
{
    if (0 == strcmp(moa->name, name))
    {
        moa->func = func;
        return 1;
    }
    return 0;
}



static void actor_hits_actor(struct MapObj * hitter, struct MapObj * hitted)
{
    struct MapObjDef * mod_hitter = get_map_object_def(hitter->type);
    struct MapObjDef * mod_hitted = get_map_object_def(hitted->type);
    struct MapObj * player = get_player();
    char * msg1 = "You";
    char * msg2 = "wound";
    char * msg3 = "you";
    if      (player != hitter)
    {
        msg1 = mod_hitter->name;
        msg2 = "wounds";
    }
    if (player != hitted)
    {
        msg3 = mod_hitted->name;
    }
    uint8_t len = 1 + strlen(msg1) + 1 + strlen(msg2) + 1 + strlen(msg3) + 2;
    char msg[len];
    sprintf(msg, "\n%s %s %s.", msg1, msg2, msg3);
    update_log(msg);
    hitted->lifepoints--;
    if (0 == hitted->lifepoints)
    {
        hitted->type = mod_hitted->corpse_id;
        if (player == hitted)
        {
            update_log(" You die.");
            return;
        }
        update_log(" It dies.");
        if (player == hitter)
        {
            world.score = world.score + mod_hitted->lifepoints;
        }
    }
}



static void playerbonus_wait()
{
        update_log("\nYou wait.");
}



static void playerbonus_move(char d, uint8_t passable)
{
    char * dsc_dir = "north";
    if      ('E' == d)
    {
        dsc_dir = "east" ;
    }
    else if ('S' == d)
    {
        dsc_dir = "south";
    }
    else if ('W' == d)
    {
        dsc_dir = "west" ;
    }
    char * dsc_move = "You move ";
    if (0 == passable)
    {
        dsc_move = "You fail to move ";
    }
    char msg[strlen(dsc_move) + strlen (dsc_dir) + 3];
    sprintf(msg, "\n%s%s.", dsc_move, dsc_dir);
    update_log(msg);
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



static void playerbonus_use(uint8_t no_object, uint8_t wrong_object)
{
    if      (no_object)
    {
        update_log("\nYou try to use an object, but you own none.");
        return;
    }
    else if (wrong_object)
    {
        update_log("\nYou try to use this object, but fail.");
        return;
    }
    update_log("\nYou consume MAGIC MEAT.");
}



extern void init_map_object_actions(char * path)
{
    char * f_name = "init_map_object_actions()";
    FILE * file = try_fopen(path, "r", f_name);
    uint16_t linemax = textfile_sizes(file, NULL);
    char line[linemax + 1];
    struct MapObjAct ** moa_ptr_ptr = &world.map_obj_acts;
    char * delim = " ";
    while (try_fgets(line, linemax + 1, file, f_name))
    {
        if ('\n' == line[0] || 0 == line[0])
        {
            break;
        }
        struct MapObjAct * moa = try_malloc(sizeof(struct MapObjAct), f_name);
        moa->id = atoi(strtok(line, delim));
        moa->effort = atoi(strtok(NULL, delim));
        char * funcname = strtok(NULL, "\n");
        uint8_t len_name = strlen(funcname) + 1;
        moa->name = try_malloc(len_name, f_name);
        memcpy(moa->name, funcname, len_name);
        if (!(   try_func_name(moa, "move", actor_move)
              || try_func_name(moa, "pick_up", actor_pick)
              || try_func_name(moa, "drop", actor_drop)
              || try_func_name(moa, "use", actor_use)))
        {
            moa->func = actor_wait;
        }
        moa->next = NULL;
        * moa_ptr_ptr = moa;
        moa_ptr_ptr = &moa->next;
    }
    try_fclose(file, f_name);
    set_cleanup_flag(CLEANUP_MAP_OBJECT_ACTS);
}



extern void free_map_object_actions(struct MapObjAct * moa)
{
    if (NULL == moa)
    {
        return;
    }
    free(moa->name);
    free_map_object_actions(moa->next);
    free(moa);
}



extern uint8_t get_moa_id_by_name(char * name)
{
    struct MapObjAct * moa = world.map_obj_acts;
    while (NULL != moa)
    {
        if (0 == strcmp(moa->name, name))
        {
            break;
        }
        moa = moa->next;
    }
    exit_err(NULL==moa, "get_moa_id_by_name() did not find map object action.");
    return moa->id;
}



extern void actor_wait(struct MapObj * mo)
{
    if (mo == get_player())
    {
        playerbonus_wait();
    }
}



extern void actor_move(struct MapObj * mo)
{
    char d = mo->arg;
    struct yx_uint16 target = mv_yx_in_dir(d, mo->pos);
    struct MapObj * other_mo;
    for (other_mo = world.map_objs; other_mo != 0; other_mo = other_mo->next)
    {
        if (0 == other_mo->lifepoints || other_mo == mo)
        {
            continue;
        }
        if (yx_uint16_cmp(&target, &other_mo->pos))
        {
            actor_hits_actor(mo, other_mo);
            return;
        }
    }
    uint8_t passable = is_passable(target);
    if (passable)
    {
        set_object_position(mo, target);
    }
    if (mo == get_player())
    {
        playerbonus_move(d, passable);
    }
}



extern void actor_drop(struct MapObj * mo)
{
    uint8_t owns_none = (NULL == mo->owns);
    if (!owns_none)
    {
        uint8_t select = mo->arg;
        struct MapObj * owned = mo->owns;
        uint8_t i = 0;
        for (; i != select; i++, owned = owned->next);
        own_map_object(&world.map_objs, &mo->owns, owned->id);
    }
    if (mo == get_player())
    {
        playerbonus_drop(owns_none);
    }
}



extern void actor_pick(struct MapObj * mo)
{
    struct MapObj * picked;
    for (picked = world.map_objs; NULL != picked; picked = picked->next)
    {
        if (picked != mo && yx_uint16_cmp(&picked->pos, &mo->pos))
        {
            break;
        }
    }
    if (NULL != picked)
    {
        own_map_object(&mo->owns, &world.map_objs, picked->id);
        set_object_position(picked, mo->pos);
    }
    if (mo == get_player())
    {
        playerbonus_pick(NULL != picked);
    }
}



extern void actor_use(struct MapObj * mo)
{
    uint8_t wrong_object = 1;
    uint8_t no_object = (NULL == mo->owns);
    if (!no_object)
    {
        uint8_t select = mo->arg;
        uint8_t i = 0;
        struct MapObj * selected = mo->owns;
        for (; i != select; i++, selected = selected->next);
        struct MapObjDef * mod = get_map_object_def(selected->type);
        if (!strcmp("MAGIC MEAT", mod->name))
        {
            wrong_object = 0;
            struct MapObj * next = selected->next;
            free(selected);
            if (0 < select)
            {
                select--;
                selected = mo->owns;
                for (i = 0; i != select; i++, selected = selected->next);
                selected->next = next;
            }
            else
            {
                mo->owns = next;
            }
            mo->lifepoints++;
        }
    }
    if (mo == get_player())
    {
        playerbonus_use(no_object, wrong_object);
    }
}