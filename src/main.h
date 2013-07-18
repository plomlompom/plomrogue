#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

struct WinMeta;
struct Win;
struct KeyBinding;
struct KeysWinData;
struct Map;
struct ItemDef;
struct MonsterDef;

struct World {
  char interactive;
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata;
  uint32_t seed;
  uint32_t turn;
  char * log;
  struct Map * map;
  struct ItemDef * item_def;
  struct Item * item;
  struct MonsterDef * monster_def;
  struct Monster * monster;
  struct Player * player; };

#endif
