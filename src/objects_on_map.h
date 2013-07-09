#ifndef ACTORS_H
#define ACTORS_H

#include "yx_uint16.h"

struct World;
struct Map;

struct Player {
  struct yx_uint16 pos; };

struct ChainMapObject {
  void * next;
  unsigned char name;
  struct yx_uint16 pos; };

struct Item {
  struct ChainMapObject cmo; };

struct Monster {
  struct ChainMapObject cmo;
  unsigned char hitpoints; };

extern char is_passable (struct Map *, struct yx_uint16);
extern struct yx_uint16 find_passable_pos (struct Map *);
extern void move_monster (struct World *, struct Monster *);
extern void move_player (struct World *, char);
extern void player_wait(struct World *);

#endif