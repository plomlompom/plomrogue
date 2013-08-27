/* map_object_actions.c */

#include "map_object_actions.h"
#include <stdlib.h> /* for malloc(), calloc(), free() */
#include <string.h> /* for strlen() */
#include "yx_uint16.h" /* for yx_uint16 struct, mv_yx_in_dir(), yx_uint16_cmp */
#include "misc.h" /* for update_log(), turn_over()*/
#include "map.h" /* for Map struct */
#include "main.h" /* for World struct */
#include "map_objects.h" /* for map object (definition) structs */
#include "rrand.h" /* for rrand() */



/* Log monster (described by "dsc_monster1") bumping into "monster2". */
static void monster_bumps_monster(struct World * world, char * dsc_monster1,
                                  struct Monster * monster2);

/* Decrement player HPs due to attack of monster described by "dsc_monster",
 * kill player if his HP hit zero; log the whole action.
 */
static void monster_hits_player(struct World * world, char * dsc_monster);

/* Decrement HP of "monster" hit by player, kill it if its HP hit zero; log the
 * whole action.
 */
static void player_hits_monster(struct World * world, struct Monster * monster);

/* Try moving the player in direction "d" towards coordinate "target"; log
 * success or failure of the whole action.
 */
static void try_player_move(struct World * world,
                            enum dir d, struct yx_uint16 target);



static void monster_bumps_monster(struct World * world, char * dsc_monster1,
                                  struct Monster * monster2)
{
    char * bump_dsc = " bumps into ";
    struct MapObjDef * mod = get_map_obj_def(world, monster2->map_obj.type);
    char * msg = malloc(strlen(dsc_monster1) + strlen(bump_dsc)
                        + strlen(mod->desc) + 3);
    sprintf(msg, "\n%s%s%s.", dsc_monster1, bump_dsc, mod->desc);
    update_log(world, msg);
    free(msg);
}



static void monster_hits_player(struct World * world, char * dsc_monster)
{
    char * hit_dsc = " hits you";
    char * msg = malloc(strlen(dsc_monster) + strlen(hit_dsc) + 3);
    sprintf(msg, "\n%s%s.", dsc_monster, hit_dsc);
    update_log(world, msg);
    free(msg);
    world->player->hitpoints--;
    if (0 == world->player->hitpoints)
    {
        update_log(world, "\nYou are dead.");
    }
}



static void player_hits_monster(struct World * world, struct Monster * monster)
{
    struct MapObjDef * mod = get_map_obj_def(world, monster->map_obj.type);
    char * hit_dsc = "You hit the ";
    char * monster_dsc = mod->desc;
    char * msg = malloc(strlen(hit_dsc) + strlen(monster_dsc) + 3);
    sprintf(msg, "\n%s%s.", hit_dsc, monster_dsc);
    update_log(world, msg);
    free(msg);
    monster->hitpoints--;
    if (0 == monster->hitpoints)
    {
        hit_dsc = "You kill the ";
        msg = malloc(strlen(hit_dsc) + strlen(monster_dsc) + 3);
        sprintf(msg, "\n%s%s.", hit_dsc, monster_dsc);
        update_log(world, msg);
        free(msg);
        struct MonsterDef * md = (struct MonsterDef * ) mod;
        struct Item * corpse = malloc(sizeof(struct Item));
        corpse->map_obj.type = md->corpse_id;
        corpse->map_obj.pos = monster->map_obj.pos;
        corpse->map_obj.next = world->item;
        world->item = corpse;
        if (world->monster == monster)
        {
            world->monster = world->monster->map_obj.next;
        }
        else
        {
            struct Monster * m_prev;
            for (m_prev = world->monster;
                 m_prev->map_obj.next != monster;
                 m_prev = m_prev->map_obj.next);
            {
                m_prev->map_obj.next = monster->map_obj.next;
            }
        }
        free(monster);
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
    char * msg = malloc(strlen(dsc_move) + strlen (dsc_dir) + 3);
    sprintf(msg, "\n%s%s.", dsc_move, dsc_dir);
    update_log(world, msg);
    free(msg);
}



extern void move_monster(struct World * world, struct Monster * monster)
{
    char d = rrand() % 5;
    struct yx_uint16 t = mv_yx_in_dir(d, monster->map_obj.pos);
    struct MapObjDef * mod = get_map_obj_def(world, monster->map_obj.type);
    char * dsc = mod->desc;
    if (yx_uint16_cmp(&t, &world->player->pos))
    {
        monster_hits_player(world, dsc);
        return;
    }
    struct Monster * other_monster;
    for (other_monster = world->monster;
         other_monster != 0;
         other_monster = other_monster->map_obj.next)
    {
        if (other_monster == monster)
        {
            continue;
        }
        if (yx_uint16_cmp(&t, &other_monster->map_obj.pos))
        {
            monster_bumps_monster(world, dsc, other_monster);
            return;
        }
    }
    if (is_passable(world->map, t))
    {
        monster->map_obj.pos = t;
    }
}



extern void move_player(struct World * world, enum dir d)
{
    struct yx_uint16 t = mv_yx_in_dir(d, world->player->pos);
    struct Monster * monster;
    for (monster = world->monster;
         monster != 0;
         monster = monster->map_obj.next)
    {
        if (yx_uint16_cmp(&t, &monster->map_obj.pos))
        {
            player_hits_monster(world, monster);
            turn_over(world, d);
            return;
          }
    }
    try_player_move(world, d, t);
    turn_over(world, d);
}



extern void player_wait (struct World * world)
{
    update_log(world, "\nYou wait.");
    turn_over(world, 0);
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
