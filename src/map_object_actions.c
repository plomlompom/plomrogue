/* map_object_actions.c */

#include "map_object_actions.h"
#include <stdlib.h> /* for malloc(), calloc(), free() */
#include "yx_uint16.h" /* for yx_uint16 struct, mv_yx_in_dir(), yx_uint16_cmp */
#include "misc.h" /* for rrand(), update_log(), turn_over()*/
#include "map.h" /* for Map struct */
#include "main.h" /* for World struct */
#include "map_objects.h" /* for map object (definition) structs */



extern void move_monster(struct World * world, struct Monster * monster)
{
    char d = rrand(0, 0) % 5;
    struct yx_uint16 t = mv_yx_in_dir(d, monster->map_obj.pos);
    char * msg = malloc(100);
    struct MapObjDef * mod = get_map_obj_def(world, monster->map_obj.type);
    char * desc = mod->desc;
    char * desc_other;
    if (yx_uint16_cmp(t, world->player->pos))
    {
        sprintf(msg, "\nThe %s hits you.", desc);
        update_log(world, msg);
        world->player->hitpoints--;
        if (0 == world->player->hitpoints)
        {
            update_log(world, "\nYou are dead.");
        }
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
        if (yx_uint16_cmp(t, other_monster->map_obj.pos))
        {
            mod = get_map_obj_def(world, monster->map_obj.type);
            desc_other = mod->desc;
            sprintf(msg, "\n%s bumps into %s.", desc, desc_other);
            update_log(world, msg);
            return;
        }
    }
    free(msg);
    if (is_passable(world->map, t))
    {
        monster->map_obj.pos = t;
    }
}



extern void move_player (struct World * world, enum dir d)
{
    struct yx_uint16 t = mv_yx_in_dir(d, world->player->pos);
    struct Monster * monster;
    struct MapObjDef * mod;
    char * msg = calloc(100, sizeof(char));
    char * desc;
    for (monster = world->monster;
         monster != 0;
         monster = monster->map_obj.next)
    {
        if (yx_uint16_cmp(t, monster->map_obj.pos))
        {
            mod = get_map_obj_def(world, monster->map_obj.type);
            desc = mod->desc;
            sprintf(msg, "\nYou hit the %s.", desc);
            update_log(world, msg);
            monster->hitpoints--;
            if (0 == monster->hitpoints)
            {
                sprintf(msg, "\nYou kill the %s.", desc);
                update_log(world, msg);
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
                    m_prev->map_obj.next = monster->map_obj.next;
                }
                free(monster);
            }
            turn_over(world, d);
            return;
          }
      }
      char * msg_content = "You fail to move";
      char * dir;
      if      (NORTH == d)
      {
          dir = "north";
      }
      else if (EAST  == d)
      {
          dir = "east" ;
      }
      else if (SOUTH == d)
      {
          dir = "south";
      }
      else if (WEST  == d)
      {
          dir = "west" ;
      }
      if (is_passable(world->map, t))
      {
          msg_content = "You move";
          world->player->pos = t;
      }
      sprintf(msg, "\n%s %s.", msg_content, dir);
      update_log(world, msg);
      free(msg);
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

