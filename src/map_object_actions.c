/* map_object_actions.c */

#include "map_object_actions.h"
#include <string.h> /* for strlen() */
#include "yx_uint16.h" /* for yx_uint16 struct, mv_yx_in_dir(), yx_uint16_cmp */
#include "misc.h" /* for update_log(), turn_over() */
#include "map.h" /* for Map struct */
#include "main.h" /* for World struct */
#include "map_objects.h" /* for structs MapObj, MapObjDef,
                          * get_map_object_def()
                          */
#include "rrand.h" /* for rrand() */
#include "command_db.h" /* for get_command_id() */



/* Log monster (described by "dsc_monster1") bumping into "monster2". */
static void monster_bumps_monster(struct World * world, char * dsc_monster1,
                                  struct MapObj * monster2);

/* Decrement player HPs due to attack of monster described by "dsc_monster",
 * kill player if his HP hit zero; log the whole action.
 */
static void monster_hits_player(struct World * world, char * dsc_monster);

/* Decrement HP of "monster" hit by player, kill it if its HP hit zero, create a
 * corpse and increment player's score by the amount of hitpoints the monster
 * started with; log the whole action.
 */
static void player_hits_monster(struct World * world, struct MapObj * monster);

/* Try moving the player in direction "d" towards coordinate "target"; log
 * success or failure of the whole action.
 */
static void try_player_move(struct World * world,
                            enum dir d, struct yx_uint16 target);



static void monster_bumps_monster(struct World * world, char * dsc_monster1,
                                  struct MapObj * monster2)
{
    char * bump_dsc = " bumps into ";
    struct MapObjDef * mod = get_map_object_def(world, monster2->type);
    char msg[strlen(dsc_monster1) + strlen(bump_dsc) + strlen(mod->name) + 3];
    sprintf(msg, "\n%s%s%s.", dsc_monster1, bump_dsc, mod->name);
    update_log(world, msg);
}



static void monster_hits_player(struct World * world, char * dsc_monster)
{
    char * hit_dsc = " hits you";
    char msg[strlen(dsc_monster) + strlen(hit_dsc) + 3];
    sprintf(msg, "\n%s%s.", dsc_monster, hit_dsc);
    update_log(world, msg);
    world->player->hitpoints--;

    if (0 == world->player->hitpoints)
    {
        update_log(world, "\nYou are dead.");
    }
}



static void player_hits_monster(struct World * world, struct MapObj * monster)
{
    struct MapObjDef * mod = get_map_object_def(world, monster->type);
    char * hit_dsc = "You hit the ";
    char * monster_dsc = mod->name;
    char hitmsg[strlen(hit_dsc) + strlen(monster_dsc) + 3];
    sprintf(hitmsg, "\n%s%s.", hit_dsc, monster_dsc);
    update_log(world, hitmsg);
    monster->lifepoints--;
    if (0 == monster->lifepoints)
    {
        hit_dsc = "You kill the ";
        char kill_msg[strlen(hit_dsc) + strlen(monster_dsc) + 3];
        sprintf(kill_msg, "\n%s%s.", hit_dsc, monster_dsc);
        update_log(world, kill_msg);
        struct MapObjDef * md = mod;
        monster->type = md->corpse_id;
        uint8_t score = md->lifepoints;
        world->score = world->score + score;
    }
}



static void try_player_move(struct World * world,
                            enum dir d, struct yx_uint16 target)
{
    char * dsc_dir;
    if      (NORTH == d)
    {
        dsc_dir = "north";
    }
    else if (EAST  == d)
    {
        dsc_dir = "east" ;
    }
    else if (SOUTH == d)
    {
        dsc_dir = "south";
    }
    else if (WEST  == d)
    {
        dsc_dir = "west" ;
    }
    char * dsc_move = "You fail to move ";
    if (is_passable(world->map, target))
    {
        dsc_move = "You move ";
        world->player->pos = target;
    }
    char msg[strlen(dsc_move) + strlen (dsc_dir) + 3];
    sprintf(msg, "\n%s%s.", dsc_move, dsc_dir);
    update_log(world, msg);
}



extern void move_monster(struct World * world, struct MapObj * monster)
{
    char d = rrand() % 5;
    struct yx_uint16 t = mv_yx_in_dir(d, monster->pos);
    struct MapObjDef * mod = get_map_object_def(world, monster->type);
    char * dsc = mod->name;
    if (yx_uint16_cmp(&t, &world->player->pos))
    {
        monster_hits_player(world, dsc);
        return;
    }
    struct MapObj * other_monster;
    for (other_monster = world->map_objs;
         other_monster != 0;
         other_monster = other_monster->next)
    {
        if (0 == other_monster->lifepoints || other_monster == monster)
        {
            continue;
        }
        if (yx_uint16_cmp(&t, &other_monster->pos))
        {
            monster_bumps_monster(world, dsc, other_monster);
            return;
        }
    }
    if (is_passable(world->map, t))
    {
        monster->pos = t;
    }
}



extern void move_player(struct World * world, enum dir d)
{
    char * action_dsc_prototype = "player_";
    uint8_t len = strlen(action_dsc_prototype);
    char action_dsc[len + 2];
    memcpy(action_dsc, action_dsc_prototype, len);
    if      (NORTH == d)
    {
        action_dsc[len] = 'u';
    }
    else if (SOUTH == d)
    {
        action_dsc[len] = 'd';
    }
    else if (WEST  == d)
    {
        action_dsc[len] = 'l';
    }
    else if (EAST  == d)
    {
        action_dsc[len] = 'r';
    }
    action_dsc[len + 1] = '\0';
    uint8_t action_id = get_command_id(world, action_dsc);
    struct yx_uint16 t = mv_yx_in_dir(d, world->player->pos);
    struct MapObj * monster;
    for (monster = world->map_objs;
         monster != 0;
         monster = monster->next)
    {
        if (0 < monster->lifepoints && yx_uint16_cmp(&t, &monster->pos))
        {
            player_hits_monster(world, monster);
            turn_over(world, action_id);
            return;
          }
    }
    try_player_move(world, d, t);
    turn_over(world, action_id);
}



extern void player_wait (struct World * world)
{
    update_log(world, "\nYou wait.");
    turn_over(world, get_command_id(world, "wait"));
}



extern char is_passable (struct Map * map, struct yx_uint16 pos)
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
