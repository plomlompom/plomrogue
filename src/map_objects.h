#ifndef MAP_OBJECTS_H
#define MAP_OBJECTS_H

#include <stdio.h>
#include "yx_uint16.h"

struct World;
struct Map;

struct Player {
  struct yx_uint16 pos;
  unsigned char hitpoints; };

struct MapObj {
  void * next;
  char type;
  struct yx_uint16 pos; };

struct MapObjDef {
  struct MapObjDef * next;
  char id;
  char mapchar;
  char * desc; };

struct Item {
  struct MapObj map_obj; };

struct Monster {
  struct MapObj map_obj;
  unsigned char hitpoints; };

struct ItemDef {
  struct MapObjDef map_obj_def; };

struct MonsterDef {
  struct MapObjDef map_obj_def;
  unsigned char hitpoints_start; };

extern void readwrite_map_objects_dummy (void *, FILE *);
extern void write_map_objects_monsterdata (void *, FILE *);
extern void write_map_objects (void * start, FILE *, void (*) (void *, FILE *) );
extern void read_map_objects_monsterdata (void *, FILE *);
extern void read_map_objects (void *, FILE *, size_t, void (*) (void *, FILE *) );
extern void build_map_objects_monsterdata (struct MapObjDef *, void *);
extern void build_map_objects_itemdata (struct MapObjDef *, void *);
extern void * build_map_objects (struct World *, void *, char, unsigned char, size_t,
                                 void (*) (struct MapObjDef *, void *));
extern struct MapObjDef * get_map_obj_def (struct World *, char);

extern struct yx_uint16 find_passable_pos (struct Map *);
extern char is_passable (struct Map *, struct yx_uint16);
extern void move_monster (struct World *, struct Monster *);
extern void move_player (struct World *, char);
extern void player_wait(struct World *);

#endif
