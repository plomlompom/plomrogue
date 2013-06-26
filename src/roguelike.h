#ifndef ROGUELIKE_H
#define ROGUELIKE_H

#include "yx_uint16.h"
#include "windows.h"

struct World {
  char interactive;
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata;
  uint32_t seed;
  uint32_t turn;
  char * log;
  struct Map * map;
  struct Monster * monster;
  struct Player * player; };

struct Map {
  struct yx_uint16 size;
  struct yx_uint16 offset;
  char * cells; };

uint16_t rrand(char, uint32_t);
void update_log (struct World *, char *);

struct Map init_map ();
void map_scroll (struct Map *, char);

void turn_over (struct World *, char);
void save_game(struct World *);

void toggle_window (struct WinMeta *, struct Win *);
void scroll_pad (struct WinMeta *, char);
void growshrink_active_window (struct WinMeta *, char);

unsigned char meta_keys(int, struct World *, struct WinMeta *, struct Win *, struct Win *, struct Win *, struct Win *);

#endif
