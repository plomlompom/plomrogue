#ifndef ACTORS_H
#define ACTORS_H

#include <stdint.h>
#include "yx_uint16.h"

struct World;
struct Map;

struct Player {
  struct yx_uint16 pos; };

struct Monster {
  struct Monster * next;
  char name;
  struct yx_uint16 pos; };

extern char is_passable (struct Map *, struct yx_uint16);
extern void move_monster (struct World *, struct Monster *);
extern void move_player (struct World *, char);
extern void player_wait(struct World *);

#endif
