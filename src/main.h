#ifndef ROGUELIKE_H
#define ROGUELIKE_H

#include <stdint.h>

struct WinMeta;
struct Win;
struct KeyBinding;
struct KeysWinData;
struct Map;

struct World {
  char interactive;
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata;
  uint32_t seed;
  uint32_t turn;
  char * log;
  struct Map * map;
  struct Item * item;
  struct Monster * monster;
  struct Player * player; };

#endif
