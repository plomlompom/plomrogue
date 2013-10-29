/* map_object_actions.c */

#include "map_object_actions.h"
#include <string.h> /* for strlen() */
#include "yx_uint16.h" /* for yx_uint16 struct, mv_yx_in_dir(),
                        * yx_uint16_cmp()
                        */
#include "map_objects.h" /* for MapObj, MapObjDef structs, get_player(),
                          * set_object_position(), own_map_object()
                          */
#include "misc.h" /* for update_log(), turn_over() */
#include "map.h" /* for Map struct */
#include "main.h" /* for World struct */
#include "command_db.h" /* for get_command_id() */



/* One actor "wounds" another actor, decrementing his lifepoints and, if they
 * reach zero in the process, killing it. Generates appropriate log message.
 */
static void actor_hits_actor(struct World * world, struct MapObj * hitter,
                             struct MapObj * hitted);



static void actor_hits_actor(struct World * world, struct MapObj * hitter,
                             struct MapObj * hitted)
{
    struct MapObjDef * mod_hitter = get_map_object_def(world, hitter->type);
    struct MapObjDef * mod_hitted = get_map_object_def(world, hitted->type);
    struct MapObj * player = get_player(world);
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
    update_log(world, msg);
    hitted->lifepoints--;
    if (0 == hitted->lifepoints)
    {
        hitted->type = mod_hitted->corpse_id;
        if (player == hitted)
        {
            update_log(world, " You die.");
        }
        else
        {
            update_log(world, " It dies.");
            if (player == hitter)
            {
                world->score = world->score + mod_hitted->lifepoints;
            }
        }
    }
}



extern uint8_t move_actor(struct World * world, struct MapObj * actor,
                          enum dir d)
{
    struct yx_uint16 target = mv_yx_in_dir(d, actor->pos);
    struct MapObj * other_actor;
    for (other_actor = world->map_objs;
         other_actor != 0;
         other_actor = other_actor->next)
    {
        if (0 == other_actor->lifepoints || other_actor == actor)
        {
            continue;
        }
        if (yx_uint16_cmp(&target, &other_actor->pos))
        {
            actor_hits_actor(world, actor, other_actor);
            return 2;
        }
    }
    if (is_passable(world->map, target))
    {
        set_object_position(actor, target);
        return 0;
    }
    return 1;
}



extern void move_player(struct World * world, enum dir d)
{
    char * dsc_dir;
    char * action_dsc_prototype = "player_";
    uint8_t len_action_dsc_prototype = strlen(action_dsc_prototype);
    char action_dsc[len_action_dsc_prototype + 2];
    memcpy(action_dsc, action_dsc_prototype, len_action_dsc_prototype);
    if      (NORTH == d)
    {
        dsc_dir = "north";
        action_dsc[len_action_dsc_prototype] = 'u';
    }
    else if (EAST  == d)
    {
        dsc_dir = "east" ;
        action_dsc[len_action_dsc_prototype] = 'r';
    }
    else if (SOUTH == d)
    {
        dsc_dir = "south";
        action_dsc[len_action_dsc_prototype] = 'd';
    }
    else if (WEST  == d)
    {
        dsc_dir = "west" ;
        action_dsc[len_action_dsc_prototype] = 'l';
    }
    action_dsc[len_action_dsc_prototype + 1] = '\0';
    uint8_t res = move_actor(world, get_player(world), d);
    if (1 >= res)
    {
        char * dsc_move = "You fail to move ";
        if   (0 == res)
        {
            dsc_move = "You move ";
        }
        char msg[strlen(dsc_move) + strlen (dsc_dir) + 3];
        sprintf(msg, "\n%s%s.", dsc_move, dsc_dir);
        update_log(world, msg);
    }
    turn_over(world, get_command_id(world, action_dsc));
}



extern void player_wait(struct World * world)
{
    update_log(world, "\nYou wait.");
    turn_over(world, get_command_id(world, "wait"));
}



extern char is_passable(struct Map * map, struct yx_uint16 pos)
{
    char passable = 0;
    if (0 <= pos.x && pos.x < map->size.x && 0 <= pos.y && pos.y < map->size.y)
    {
        if ('.' == map->cells[pos.y * map->size.x + pos.x])
        {
            passable = 1;
        }
    }
    return passable;
}



extern void player_drop(struct World * world)
{
    struct MapObj * player = get_player(world);
    if (NULL == player->owns)
    {
        update_log(world, "\nYou try to drop an object, but you own none.");
        world->old_inventory_select = 0;
    }
    else
    {
        world->old_inventory_select = world->inventory_select;
        struct MapObj * owned = player->owns;
        uint8_t i = 0;
        for (; i != world->inventory_select; i++, owned = owned->next);
        if (0 < world->inventory_select)
        {
            world->inventory_select--;
        }
        own_map_object(&world->map_objs, &player->owns, owned->id);
        update_log(world, "\nYou drop an object.");
    }
    turn_over(world, get_command_id(world, "drop"));
}



extern void player_pick(struct World * world)
{
    struct MapObj * player = get_player(world);
    struct MapObj * picked;
    for (picked = world->map_objs; NULL != picked; picked = picked->next)
    {
        if (picked != player && yx_uint16_cmp(&picked->pos, &player->pos))
        {
            break;
        }
    }
    if (NULL == picked)
    {
        update_log(world, "\nYou try to pick up an object, but there is none.");
    }
    else
    {
        own_map_object(&player->owns, &world->map_objs, picked->id);
        set_object_position(picked, player->pos);
        update_log(world, "\nYou pick up an object.");
    }
    turn_over(world, get_command_id(world, "pick"));
}
