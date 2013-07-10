#ifndef ROGUELIKE_H
#define ROGUELIKE_H

#include <stdint.h>
#include "yx_uint16.h"

struct WinMeta;
struct Win;
struct KeyBinding;
struct KeysWinData;

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

struct Map {
  struct yx_uint16 size;
  struct yx_uint16 offset;
  char * cells; };

extern uint16_t rrand(char, uint32_t);
extern void update_log (struct World *, char *);

extern struct Map init_map ();
extern void map_scroll (struct Map *, char, struct yx_uint16);
uint16_t center_offset (uint16_t, uint16_t, uint16_t);
extern void map_center_player (struct Map *, struct Player *, struct yx_uint16);

extern void turn_over (struct World *, char);
extern void save_game(struct World *);

extern void toggle_window (struct WinMeta *, struct Win *);
extern void scroll_pad (struct WinMeta *, char);
extern void growshrink_active_window (struct WinMeta *, char);

extern unsigned char meta_keys(int, struct World *, struct WinMeta *, struct Win *, struct Win *, struct Win *, struct Win *);

#endif
