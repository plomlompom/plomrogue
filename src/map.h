#ifndef MAP_H
#define MAP_H

#include "yx_uint16.h"

struct Player;

struct Map {
  struct yx_uint16 size;
  struct yx_uint16 offset;
  char * cells; };

extern struct Map init_map ();
extern void map_scroll (struct Map *, char, struct yx_uint16);
extern void map_center_player (struct Map *, struct Player *, struct yx_uint16);

#endif
