#include "actors.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "yx_uint16.h"
#include "roguelike.h"

extern char is_passable (struct Map * map, uint16_t y, uint16_t x) {
// Check if coordinate on (or beyond) map is accessible to actor movement.
  char passable = 0;
  if (0 <= x && x < map->size.x && 0 <= y && y < map->size.y)
    if ('.' == map->cells[y * map->size.x + x])
      passable = 1;
  return passable; }

extern void move_monster (struct World * world, struct Monster * monster) {
// Move monster in random direction, trigger fighting when hindered by player/monster.
  char d = rrand(0, 0) % 5;
  struct yx_uint16 t = mv_yx_in_dir (d, monster->pos);
  if (yx_uint16_cmp (t, world->player->pos)) {
    update_log (world, "\nThe monster hits you.");
    return; }
  struct Monster * other_monster;
  for (other_monster = world->monster; other_monster != 0; other_monster = other_monster->next) {
    if (other_monster == monster)
      continue;
    if (yx_uint16_cmp (t, other_monster->pos)) {
      update_log (world, "\nMonster hits monster.");
      return; } }
  if (is_passable(world->map, t.y, t.x))
    monster->pos = t; }

extern void move_player (struct World * world, char d) {
// Move player in direction d, increment turn counter and update log.
  struct yx_uint16 t = mv_yx_in_dir (d, world->player->pos);
  struct Monster * monster;
  for (monster = world->monster; monster != 0; monster = monster->next)
    if (yx_uint16_cmp (t, monster->pos)) {
      update_log (world, "\nYou hit the monster.");
      turn_over (world, d);
      return; }
  char * msg = calloc(25, sizeof(char));
  char * msg_content = "You fail to move";
  char * dir;
  if      (NORTH == d) dir = "north";
  else if (EAST  == d) dir = "east" ;
  else if (SOUTH == d) dir = "south";
  else if (WEST  == d) dir = "west" ;
  if (is_passable(world->map, t.y, t.x)) {
    msg_content = "You move";
    world->player->pos = t; }
  sprintf(msg, "\n%s %s.", msg_content, dir);
  update_log (world, msg);
  free(msg);
  turn_over (world, d); }

extern void player_wait (struct World * world) {
// Make player wait one turn.
  update_log (world, "\nYou wait.");
  turn_over (world, 0); }
