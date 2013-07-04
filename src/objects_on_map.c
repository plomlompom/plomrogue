#include "objects_on_map.h"
#include <stdlib.h>
#include <stdio.h>
#include "yx_uint16.h"
#include "roguelike.h"

extern char is_passable (struct Map * map, struct yx_uint16 pos) {
// Check if coordinate on (or beyond) map is accessible to actor movement.
  char passable = 0;
  if (0 <= pos.x && pos.x < map->size.x && 0 <= pos.y && pos.y < map->size.y)
    if ('.' == map->cells[pos.y * map->size.x + pos.x])
      passable = 1;
  return passable; }

extern struct yx_uint16 find_passable_pos (struct Map * map) {
// Return a random passable position on map.
  struct yx_uint16 pos;
  for (pos.y = pos.x = 0; 0 == is_passable(map, pos);) {
      pos.y = rrand(0, 0) % map->size.y;
      pos.x = rrand(0, 0) % map->size.x; }
  return pos; }

extern void move_monster (struct World * world, struct Monster * monster) {
// Move monster in random direction, trigger fighting when hindered by player/monster.
  char d = rrand(0, 0) % 5;
  struct yx_uint16 t = mv_yx_in_dir (d, monster->cmo.pos);
  if (yx_uint16_cmp (t, world->player->pos)) {
    update_log (world, "\nThe monster hits you.");
    return; }
  struct Monster * other_monster;
  for (other_monster = world->monster; other_monster != 0; other_monster = other_monster->cmo.next) {
    if (other_monster == monster)
      continue;
    if (yx_uint16_cmp (t, other_monster->cmo.pos)) {
      update_log (world, "\nMonster bumps into monster.");
      return; } }
  if (is_passable(world->map, t))
    monster->cmo.pos = t; }

extern void move_player (struct World * world, char d) {
// Move player in direction d, update log and turn over to the enemy.
  struct yx_uint16 t = mv_yx_in_dir (d, world->player->pos);
  struct Monster * monster;
  for (monster = world->monster; monster != 0; monster = monster->cmo.next)
    if (yx_uint16_cmp (t, monster->cmo.pos)) {
      update_log (world, "\nYou hit the monster.");
      monster->hitpoints--;
      if (0 == monster->hitpoints) {
        update_log (world, "\nYou kill the monster.");
        if (world->monster == monster)
          world->monster = world->monster->cmo.next;
        else {
          struct Monster * m_prev;
          for (m_prev = world->monster; m_prev->cmo.next != monster; m_prev = m_prev->cmo.next);
          m_prev->cmo.next = monster->cmo.next; }
        free(monster); }
      turn_over (world, d);
      return; }
  char * msg = calloc(25, sizeof(char));
  char * msg_content = "You fail to move";
  char * dir;
  if      (NORTH == d) dir = "north";
  else if (EAST  == d) dir = "east" ;
  else if (SOUTH == d) dir = "south";
  else if (WEST  == d) dir = "west" ;
  if (is_passable(world->map, t)) {
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
