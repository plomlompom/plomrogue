#include "misc.h"
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "keybindings.h"
#include "readwrite.h"
#include "objects_on_map.h"
#include "map.h"
#include "main.h"

uint16_t rrand(char use_seed, uint32_t new_seed) {
// Pseudo-random number generator (LGC algorithm). Use instead of rand() to ensure portable predictability.
  static uint32_t seed;
  if (0 != use_seed)
    seed = new_seed;
  seed = ((seed * 1103515245) + 12345) % 2147483648;   // Values as recommended by POSIX.1-2001 (see rand(3)).
  return (seed / 65536); }                         // Ignore least significant 16 bits (they are less random).

void update_log (struct World * world, char * text) {
// Update log by appending text, or by appending a "." if text is the same as the last one.
  static char * last_msg;
  if (0 == last_msg)
    last_msg = calloc(1, sizeof(char));
  char * new_text;
  uint16_t len_old = strlen(world->log);
  if (0 == strcmp(last_msg, text)) {
    uint16_t len_whole = len_old + 1;
    new_text = calloc(len_whole + 1, sizeof(char));
    memcpy(new_text, world->log, len_old);
    memcpy(new_text + len_old, ".", 1); }
  else {
    uint16_t len_new = strlen(text);
    uint16_t len_whole = len_old + len_new + 1;
    new_text = calloc(len_whole, sizeof(char));
    memcpy(new_text, world->log, len_old);
    memcpy(new_text + len_old, text, len_new);
    last_msg = calloc(len_new + 1, sizeof(char));
    memcpy(last_msg, text, len_new); }
  free(world->log);
  world->log = new_text; }

uint16_t center_offset (uint16_t pos, uint16_t mapsize, uint16_t framesize) {
// Return the offset for display of a map inside a frame centered on pos.
  uint16_t offset = 0;
  if (mapsize > framesize) {
    if (pos > framesize / 2) {
      if (pos < mapsize - (framesize / 2))
        offset = pos - (framesize / 2);
      else
        offset = mapsize - framesize; } }
  return offset; }

void turn_over (struct World * world, char action) {
// Record action in game record file, increment turn and move enemy.
  if (1 == world->interactive) {
    FILE * file = fopen("record", "a");
    fputc(action, file);
    fclose(file); }
  world->turn++;
  rrand(1, world->seed * world->turn);
  struct Monster * monster;
  for (monster = world->monster; monster != 0; monster = monster->cmo.next)
    move_monster(world, monster); }

void save_game(struct World * world) {
// Save game data to game file.
  FILE * file = fopen("savefile", "w");
  write_uint32_bigendian(world->seed, file);
  write_uint32_bigendian(world->turn, file);
  write_uint16_bigendian(world->player->pos.y + 1, file);
  write_uint16_bigendian(world->player->pos.x + 1, file);
  fputc(world->player->hitpoints, file);
  write_map_objects (world->monster, file, write_map_objects_monsterdata);
  write_map_objects (world->item, file, readwrite_map_objects_dummy);
  fclose(file); }

void toggle_window (struct WinMeta * win_meta, struct Win * win) {
// Toggle display of window win.
  if (0 != win->frame.curses_win)
    suspend_win(win_meta, win);
  else
    append_win(win_meta, win); }

void scroll_pad (struct WinMeta * win_meta, char dir) {
// Try to scroll pad left or right.
  if      ('+' == dir)
    reset_pad_offset(win_meta, win_meta->pad_offset + 1);
  else if ('-' == dir)
    reset_pad_offset(win_meta, win_meta->pad_offset - 1); }

void growshrink_active_window (struct WinMeta * win_meta, char change) {
// Grow or shrink active window horizontally or vertically by one cell size.
  if (0 != win_meta->active) {
    struct yx_uint16 size = win_meta->active->frame.size;
    if      (change == '-') size.y--;
    else if (change == '+') size.y++;
    else if (change == '_') size.x--;
    else if (change == '*') size.x++;
    resize_active_win (win_meta, size); } }

unsigned char meta_keys(int key, struct World * world, struct WinMeta * win_meta, struct Win * win_keys,
                        struct Win * win_map, struct Win * win_info, struct Win * win_log) {
// Call some meta program / window management actions dependent on key. Return 1 to signal quitting.
  if (key == get_action_key(world->keybindings, "quit"))
    return 1;
  else if (key == get_action_key(world->keybindings, "scroll pad right"))
    scroll_pad (win_meta, '+');
  else if (key == get_action_key(world->keybindings, "scroll pad left"))
    scroll_pad (win_meta, '-');
  else if (key == get_action_key(world->keybindings, "toggle keys window"))
    toggle_window(win_meta, win_keys);
  else if (key == get_action_key(world->keybindings, "toggle map window"))
    toggle_window(win_meta, win_map);
  else if (key == get_action_key(world->keybindings, "toggle info window"))
    toggle_window(win_meta, win_info);
  else if (key == get_action_key(world->keybindings, "toggle log window"))
    toggle_window(win_meta, win_log);
  else if (key == get_action_key(world->keybindings, "cycle forwards"))
    cycle_active_win(win_meta, 'n');
  else if (key == get_action_key(world->keybindings, "cycle backwards"))
    cycle_active_win(win_meta, 'p');
  else if (key == get_action_key(world->keybindings, "shift forwards"))
    shift_active_win(win_meta, 'f');
  else if (key == get_action_key(world->keybindings, "shift backwards"))
    shift_active_win(win_meta, 'b');
  else if (key == get_action_key(world->keybindings, "grow horizontally"))
    growshrink_active_window(win_meta, '*');
  else if (key == get_action_key(world->keybindings, "shrink horizontally"))
    growshrink_active_window(win_meta, '_');
  else if (key == get_action_key(world->keybindings, "grow vertically"))
    growshrink_active_window(win_meta, '+');
  else if (key == get_action_key(world->keybindings, "shrink vertically"))
    growshrink_active_window(win_meta, '-');
  else if (key == get_action_key(world->keybindings, "save keys"))
    save_keybindings(world);
  else if (key == get_action_key(world->keybindings, "keys nav up"))
    keyswin_move_selection (world, 'u');
  else if (key == get_action_key(world->keybindings, "keys nav down"))
    keyswin_move_selection (world, 'd');
  else if (key == get_action_key(world->keybindings, "keys mod"))
    keyswin_mod_key (world, win_meta);
  else if (key == get_action_key(world->keybindings, "map up"))
    map_scroll (world->map, NORTH, win_map->frame.size);
  else if (key == get_action_key(world->keybindings, "map down"))
    map_scroll (world->map, SOUTH, win_map->frame.size);
  else if (key == get_action_key(world->keybindings, "map right"))
    map_scroll (world->map, EAST, win_map->frame.size);
  else if (key == get_action_key(world->keybindings, "map left"))
    map_scroll (world->map, WEST, win_map->frame.size);
  else if (key == get_action_key(world->keybindings, "map center player"))
    map_center_player (world->map, world->player, win_map->frame.size);
  return 0; }
