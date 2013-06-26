#ifndef ROGUELIKE_H
#define ROGUELIKE_H

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

struct Player {
  struct yx_uint16 pos; };

struct Monster {
  struct Monster * next;
  char name;
  struct yx_uint16 pos; };

uint16_t rrand(char, uint32_t);
void update_log (struct World *, char *);

struct Map init_map ();
void map_scroll (struct Map *, char);

void turn_over (struct World *, char);
void save_game(struct World *);

char is_passable (struct Map *, uint16_t, uint16_t);
struct yx_uint16 mv_yx_in_dir (char, struct yx_uint16);
void move_monster (struct World *, struct Monster *);
void move_player (struct World *, char);
void player_wait(struct World *);

void toggle_window (struct WinMeta *, struct Win *);
void scroll_pad (struct WinMeta *, char);
void growshrink_active_window (struct WinMeta *, char);

unsigned char meta_keys(int, struct World *, struct WinMeta *, struct Win *, struct Win *, struct Win *, struct Win *);

#endif
