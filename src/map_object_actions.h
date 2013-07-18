#ifndef MAP_OBJECT_ACTIONS_H
#define MAP_OBJECT_ACTIONS_H

#include "yx_uint16.h"

struct World;
struct Map;
struct Monster;

extern char is_passable (struct Map *, struct yx_uint16);
extern void move_monster (struct World *, struct Monster *);
extern void move_player (struct World *, char);
extern void player_wait(struct World *);

#endif
