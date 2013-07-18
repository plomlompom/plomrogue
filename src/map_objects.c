#include "map_objects.h"
#include <stdlib.h>
#include <stdio.h>
#include "yx_uint16.h"
#include "readwrite.h"
#include "misc.h"
#include "map.h"
#include "main.h"

static struct MapObj * get_next_map_obj (void *, char *, size_t, struct MapObj *);

extern void readwrite_map_objects_dummy (void * dummy, FILE * file) {
// Dummy function for calls of (write|read)_map_objects on map objects without specific attributes.
  ; }

extern void write_map_objects_monsterdata (void * start, FILE * file) {
// Write to file data specific to map objects of type monster.
  struct Monster * m = (struct Monster *) start;
  fputc(m->hitpoints, file); }

extern void write_map_objects (void * start, FILE * file, void (* w_typedata) (void *, FILE *) ) {
// Write into file the map object chain starting at start, use write_type() for object-type specific data.
  struct MapObj * map_obj;
  for (map_obj = start; map_obj != 0; map_obj = map_obj->next) {
    write_uint16_bigendian(map_obj->pos.y + 1, file);
    write_uint16_bigendian(map_obj->pos.x + 1, file);
    fputc(map_obj->type, file);
    w_typedata (map_obj, file); }
  write_uint16_bigendian(0, file); }

extern void read_map_objects_monsterdata (void * start, FILE * file) {
// Read from file data speciifc to map objects of type monster.
  struct Monster * m = (struct Monster *) start;
  m->hitpoints = fgetc(file); }

static struct MapObj * get_next_map_obj (void * start, char * first, size_t size, struct MapObj * map_obj) {
// Return pointer to map object of "size". If first in chain ("first" pointing to !0), point "start" to it.
  if (* first) {
    struct MapObj * * z = start;
    map_obj = malloc(size);
    * z = map_obj;
    * first = 0; }
  else {
    map_obj->next = malloc(size);
    map_obj = map_obj->next; }
  return map_obj; }

extern void read_map_objects (void * start, FILE * file, size_t size, void (* r_typedata) (void *, FILE *) ) {
// Read from file chain of map objects starting at start, use r_typedata() for object-type specific data.
  struct MapObj * map_obj;
  uint16_t test;
  char first = 1;
  while (1) {
    test = read_uint16_bigendian(file);
    if (0 == test)
      break;
    map_obj = get_next_map_obj(start, &first, size, map_obj);
    map_obj->pos.y = test - 1;
    map_obj->pos.x = read_uint16_bigendian(file) - 1;
    map_obj->type = fgetc(file);
    r_typedata (map_obj, file); }
  if (!first)
    map_obj->next = 0; }

extern void build_map_objects_monsterdata (struct MapObjDef * map_obj_def, void * start) {
// Build data specific to map objects of type monster.
  struct Monster * m = (struct Monster *) start;
  m->map_obj.type = map_obj_def->id;
  struct MonsterDef * md = (struct MonsterDef *) map_obj_def;
  m->hitpoints = md->hitpoints_start; }

extern void build_map_objects_itemdata (struct MapObjDef * map_obj_def, void * start) {
// Build data speciifc to map objects of type data.
  struct Item * i = (struct Item *) start;
  i->map_obj.type = map_obj_def->id; }

extern void * build_map_objects (struct World * world, void * start, char def_id, unsigned char n,
                                 size_t size, void (* b_typedata) (struct MapObjDef *, void *)) {
// Build chain of n map objects starting at start, use f() for object-specific data.
  unsigned char i;
  struct MapObj * mo;
  char first = 1;
  struct MapObjDef * mod = get_map_obj_def (world, def_id);
  for (i = 0; i < n; i++) {
    mo = get_next_map_obj(start, &first, size, mo);
    mo->pos = find_passable_pos(world->map);
    b_typedata (mod, mo); }
  if (!first)
    mo->next = 0;
  return &mo->next; }

extern struct MapObjDef * get_map_obj_def (struct World * world, char def_id) {
// Get pointer to the map object definition with id "def_id".
  struct MapObjDef * d = NULL;
  for (d = (struct MapObjDef *) world->monster_def; d->id != def_id && 0 != d->next; d = d->next);
  if (d->id != def_id)
    for (d = (struct MapObjDef *) world->item_def; d->id != def_id && 0 != d->next; d = d->next);
  return d; }

extern struct yx_uint16 find_passable_pos (struct Map * map) {
// Return a random passable position on map.
  struct yx_uint16 pos;
  for (pos.y = pos.x = 0; 0 == is_passable(map, pos);) {
      pos.y = rrand(0, 0) % map->size.y;
      pos.x = rrand(0, 0) % map->size.x; }
  return pos; }

extern char is_passable (struct Map * map, struct yx_uint16 pos) {
// Check if coordinate on (or beyond) map is accessible to actor movement.
  char passable = 0;
  if (0 <= pos.x && pos.x < map->size.x && 0 <= pos.y && pos.y < map->size.y)
    if ('.' == map->cells[pos.y * map->size.x + pos.x])
      passable = 1;
  return passable; }

extern void move_monster (struct World * world, struct Monster * monster) {
// Move monster in random direction, trigger fighting when hindered by player/monster.
  char d = rrand(0, 0) % 5;
  struct yx_uint16 t = mv_yx_in_dir (d, monster->map_obj.pos);
  char * msg = malloc(100);
  struct MapObjDef * mod = get_map_obj_def (world, monster->map_obj.type);
  char * desc = mod->desc;
  char * desc_other;
  if (yx_uint16_cmp (t, world->player->pos)) {
    sprintf(msg, "\nThe %s hits you.", desc);
    update_log (world, msg);
    world->player->hitpoints--;
    if (0 == world->player->hitpoints)
      update_log (world, "\nYou are dead.");
    return; }
  struct Monster * other_monster;
  for (other_monster = world->monster; other_monster != 0; other_monster = other_monster->map_obj.next) {
    if (other_monster == monster)
      continue;
    if (yx_uint16_cmp (t, other_monster->map_obj.pos)) {
      mod = get_map_obj_def (world, monster->map_obj.type);
      desc_other = mod->desc;
      sprintf(msg, "\n%s bumps into %s.", desc, desc_other);
      update_log (world, msg);
      return; } }
  free (msg);
  if (is_passable(world->map, t))
    monster->map_obj.pos = t; }

extern void move_player (struct World * world, char d) {
// Move player in direction d, update log and turn over to the enemy.
  struct yx_uint16 t = mv_yx_in_dir (d, world->player->pos);
  struct Monster * monster;
  struct MapObjDef * mod;
  char * msg = calloc(100, sizeof(char));
  char * desc;
  for (monster = world->monster; monster != 0; monster = monster->map_obj.next)
    if (yx_uint16_cmp (t, monster->map_obj.pos)) {
      mod = get_map_obj_def (world, monster->map_obj.type);
      desc = mod->desc;
      sprintf (msg, "\nYou hit the %s.", desc);
      update_log (world, msg);
      monster->hitpoints--;
      if (0 == monster->hitpoints) {
        sprintf (msg, "\nYou kill the %s.", desc);
        update_log (world, msg);
        if (world->monster == monster)
          world->monster = world->monster->map_obj.next;
        else {
          struct Monster * m_prev;
          for (m_prev = world->monster; m_prev->map_obj.next != monster; m_prev = m_prev->map_obj.next);
          m_prev->map_obj.next = monster->map_obj.next; }
        free(monster); }
      turn_over (world, d);
      return; }
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
