#include "objects_on_map.h"
#include <stdlib.h>
#include <stdio.h>
#include "yx_uint16.h"
#include "readwrite.h"
#include "roguelike.h"

extern void readwrite_map_objects_dummy (void * dummy, FILE * file) {
// Dummy function for calls of (write|read)_map_objects on map objects without specific attributes.
  ; }

extern void write_map_objects_monsterdata (void * start, FILE * file) {
// Write to file data specific tto map objects of type monster.
  struct Monster * monster = (struct Monster *) start;
  fputc(monster->hitpoints, file); }

extern void write_map_objects (void * start, FILE * file, void (* f) (void *, FILE *) ) {
// Write into file the map object chain starting at start, use f() for object-type specific data.
  struct ChainMapObject * cmo;
  for (cmo = start; cmo != 0; cmo = cmo->next) {
    write_uint16_bigendian(cmo->pos.y + 1, file);
    write_uint16_bigendian(cmo->pos.x + 1, file);
    fputc(cmo->name, file);
    f (cmo, file); }
  write_uint16_bigendian(0, file); }

extern void read_map_objects_monsterdata (void * start, FILE * file) {
// Read from file data speciifc to map objects of type monster.
  struct Monster * monster = (struct Monster *) start;
  monster->hitpoints = fgetc(file); }

extern void read_map_objects (void * start, FILE * file, size_t size, void (* f) (void *, FILE *) ) {
// Read from file chain of map objects starting at start, use f() for object-type specific data.
  struct ChainMapObject * cmo;
  struct ChainMapObject * * z = start;
  uint16_t test;
  char still_at_start = 1;
  while (1) {
    test = read_uint16_bigendian(file);
    if (0 == test)
      break;
    if (still_at_start) {
      cmo = malloc(size);
      * z = cmo;
      still_at_start = 0; }
    else {
      cmo->next = malloc(size);
      cmo = cmo->next; }
    cmo->pos.y = test - 1;
    cmo->pos.x = read_uint16_bigendian(file) - 1;
    cmo->name = fgetc(file);
    f (cmo, file); }
  if (!still_at_start)
    cmo->next = 0; }

extern void build_map_objects_monsterdata (void * start) {
// Build data specific to map objects of type monster.
  struct Monster * monster = (struct Monster *) start;
  monster->cmo.name = 'A' + (rrand(0, 0) % 8);
  monster->hitpoints = 5; }

extern void build_map_objects_itemdata (void * start) {
// Build data speciifc to map objects of type data.
  struct Item * item = (struct Item *) start;
  item->cmo.name = '#' + (rrand(0, 0) % 4); }

extern void build_map_objects (void * start, unsigned char n, size_t size, void (* f) (void *), struct Map * map) {
// Build chain of n map objects starting at start, use f() for object-specific data.
  unsigned char i;
  struct ChainMapObject * cmo;
  struct ChainMapObject * * z = start;
  char still_at_start = 1;
  for (i = 0; i < n; i++) {
    if (still_at_start) {
      cmo = malloc(size);
      * z = cmo;
      still_at_start = 0; }
    else {
      cmo->next = malloc(size);
      cmo = cmo->next; }
    cmo->pos = find_passable_pos(map);
    f(cmo); }
  if (!still_at_start)
    cmo->next = 0; }

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
