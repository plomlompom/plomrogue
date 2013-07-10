#ifndef ACTORS_H
#define ACTORS_H

#include <stdio.h>
#include "yx_uint16.h"

struct World;
struct Map;

struct Player {
  struct yx_uint16 pos;
  unsigned char hitpoints; };

struct ChainMapObject {
  void * next;
  unsigned char name;
  struct yx_uint16 pos; };

struct Item {
  struct ChainMapObject cmo; };

struct Monster {
  struct ChainMapObject cmo;
  unsigned char hitpoints; };

extern void readwrite_map_objects_dummy (void *, FILE *);
extern void write_map_objects_monsterdata (void *, FILE *);
extern void write_map_objects (void * start, FILE *, void (*) (void *, FILE *) );
extern void read_map_objects_monsterdata (void *, FILE *);
extern void read_map_objects (void *, FILE *, size_t, void (*) (void *, FILE *) );
extern void build_map_objects_monsterdata (void *);
extern void build_map_objects_itemdata (void *);
extern void build_map_objects (void *, unsigned char, size_t, void (*) (void *), struct Map *);

extern struct yx_uint16 find_passable_pos (struct Map *);
extern char is_passable (struct Map *, struct yx_uint16);
extern void move_monster (struct World *, struct Monster *);
extern void move_player (struct World *, char);
extern void player_wait(struct World *);

#endif
