#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <ncurses.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "windows.h"
#include "draw_wins.h"
#include "roguelike.h"
#include "keybindings.h"

uint16_t rrand(char use_seed, uint32_t new_seed) {
// Pseudo-random number generator (LGC algorithm). Use instead of rand() to ensure portable predictability.
  static uint32_t seed = 0;
  if (0 != use_seed)
    seed = new_seed;
  seed = ((seed * 1103515245) + 12345) % 2147483648;   // Values as recommended by POSIX.1-2001 (see rand(3)).
  return (seed / 65536); }                         // Ignore least significant 16 bits (they are less random).

uint32_t load_seed() {
// Load seed integer from seed file.
  uint32_t seed;
  const uint16_t nchar = UCHAR_MAX + 1;
  FILE * file = fopen("seed", "r");
  unsigned char a = fgetc(file);
  unsigned char b = fgetc(file);
  unsigned char c = fgetc(file);
  unsigned char d = fgetc(file);
  seed = (a * nchar * nchar * nchar) + (b * nchar * nchar) + (c * nchar) + d;
  fclose(file);
  return seed; }

void save_seed(uint32_t seed) {
// Save seed integer to seed file.
  const uint16_t nchar = UCHAR_MAX + 1;
  unsigned char a = seed / (nchar * nchar * nchar);
  unsigned char b = (seed - (a * nchar * nchar * nchar)) / (nchar * nchar);
  unsigned char c = (seed - ((a * nchar * nchar * nchar) + (b * nchar * nchar))) / nchar;
  unsigned char d = seed % nchar;
  FILE * file = fopen("seed", "w");
  fputc(a, file);
  fputc(b, file);
  fputc(c, file);
  fputc(d, file);
  fclose(file); }

void toggle_window (struct WinMeta * win_meta, struct Win * win) {
// Toggle display of window win.
  if (0 != win->curses)
    suspend_window(win_meta, win);
  else
    append_window(win_meta, win); }

void growshrink_active_window (struct WinMeta * win_meta, char change) {
// Grow or shrink active window horizontally or vertically by one cell size.
  if (0 != win_meta->active) {
    uint16_t height = win_meta->active->height;
    uint16_t width = win_meta->active->width;
    if      (change == '-')
      height--;
    else if (change == '+')
      height++;
    else if (change == '_')
      width--;
    else if (change == '*')
      width++;
    resize_active_window (win_meta, height, width); } }

struct Map init_map (uint32_t seed) {
// Initialize map with some experimental start values.
  rrand(1, seed);
  struct Map map;
  map.width = 64;
  map.height = 64;
  map.offset_x = 0;
  map.offset_y = 0;
  map.cells = malloc(map.width * map.height);
  uint16_t x, y, ran;
  char terrain;
  for (y = 0; y < map.height; y++)
    for (x = 0; x < map.width; x++) {
      terrain = '.';
      ran = rrand(0, 0);
      if (   0 == ran % ((x*x) / 3 + 1)
          || 0 == ran % ((y*y) / 3 + 1)
          || 0 == ran % ((map.width - x - 1) * (map.width - x - 1) / 3 + 1)
          || 0 == ran %((map.height - y - 1) * (map.height - y - 1) / 3 + 1))
        terrain = ' ';
      map.cells[(y * map.width) + x] = terrain; }
  return map; }

void map_scroll (struct Map * map, char dir) {
// Scroll map into direction dir if possible by changing the offset.
  if      ('n' == dir && map->offset_y > 0)
    map->offset_y--;
  else if ('s' == dir)
    map->offset_y++;
  else if ('w' == dir && map->offset_x > 0)
    map->offset_x--;
  else if ('e' == dir)
    map->offset_x++; }

void next_turn (struct World * world) {
// Increment turn and move enemy.
  world->turn++;
  char d = rrand(0, 0) % 5;
  uint16_t ty = world->monster->y;
  uint16_t tx = world->monster->x;
  if (1 == d)
    ty++;
  else if (2 == d)
    ty--;
  else if (3 == d)
    tx++;
  else if (4 == d)
    tx--;
  if (tx == world->player->x && ty == world->player->y)
    update_log(world, "\nThe monster hits you.");
  else if (is_passable(world, tx, ty)) {
    world->monster->y = ty;
    world->monster->x = tx; } }

void update_log (struct World * world, char * text) {
// Update log with new text to be appended.
  char * new_text;
  uint16_t len_old = strlen(world->log);
  uint16_t len_new = strlen(text);
  uint16_t len_whole = len_old + len_new + 1;
  new_text = calloc(len_whole, sizeof(char));
  memcpy(new_text, world->log, len_old);
  memcpy(new_text + len_old, text, len_new);
  free(world->log);
  world->log = new_text; }

char is_passable (struct World * world, uint16_t x, uint16_t y) {
// Check if coordinate on (or beyond) map is accessible to movement.
  char passable = 0;
  if (0 <= x && x < world->map->width && 0 <= y && y < world->map->height)
    if ('.' == world->map->cells[y * world->map->width + x])
      passable = 1;
  return passable; }

void move_player (struct World * world, char d) {
// Move player in direction d, increment turn counter and update log.
  static char prev = 0;
  char success = 0;
  char * dir;
  uint16_t ty = world->player->y;
  uint16_t tx = world->player->x;
  if ('s' == d) {
    dir = "south";
    ty++; }
  if ('n' == d) {
    dir = "north";
    ty--; }
  if ('w' == d) {
    dir = "west";
    tx--; }
  if ('e' == d) {
    dir = "east";
    tx++; }
  if (ty == world->monster->y && tx == world->monster->x)
    success = 2;
  else if (is_passable(world, tx, ty)) {
    success = 1;
    world->player->y = ty;
    world->player->x = tx; }
  if (success * d == prev)
    update_log (world, ".");
  else {
    if (2 == success)
      update_log (world, "\nYou hit the monster.");
    else {
      char * msg = calloc(25, sizeof(char));
      char * msg_content = "You fail to move";
      if (1 == success)
        msg_content = "You move";
      sprintf(msg, "\n%s %s.", msg_content, dir);
      update_log (world, msg);
      free(msg); } }
  prev = success * d;
  next_turn (world); }

void player_wait (struct World * world) {
// Make player wait one turn.
  next_turn (world);
  update_log (world, "\nYou wait."); }

int main (int argc, char *argv[]) {
  uint32_t seed = time(NULL);
  int opt;
  while ((opt = getopt(argc, argv, "l")) != -1) {
    switch (opt) {
      case 'l':
	seed = load_seed();
        break;
      default:
        exit(EXIT_FAILURE); } }

  struct World world;
  init_keybindings(&world);
  world.turn = 0;
  world.log = calloc(1, sizeof(char));
  update_log (&world, "Start!");
  struct Map map = init_map(seed);
  world.map = &map;
  struct Player player;
  player.y = 8;
  player.x = 8;
  world.player = &player;
  struct Monster monster;
  monster.y = 55;
  monster.x = 55;
  world.monster = &monster;

  WINDOW * screen = initscr();
  noecho();
  curs_set(0);
  keypad(screen, TRUE);
  raw();
  struct WinMeta win_meta = init_win_meta(screen);
  struct Win win_keys = init_window(&win_meta, "Keys", &world, draw_keys_win);
  struct Win win_map = init_window(&win_meta, "Map", &world, draw_map_win);
  struct Win win_info = init_window(&win_meta, "Info", &world, draw_info_win);
  struct Win win_log = init_window(&win_meta, "Log", &world, draw_log_win);

  int key;
  while (1) {
    draw_all_windows (&win_meta);
    key = getch();
    if      (key == get_action_key(world.keybindings, "quit"))
      break;
    else if (key == get_action_key(world.keybindings, "scroll pad right"))
      scroll_pad (&win_meta, '+');
    else if (key == get_action_key(world.keybindings, "scroll pad left"))
      scroll_pad (&win_meta, '-');
    else if (key == get_action_key(world.keybindings, "toggle keys window"))
      toggle_window(&win_meta, &win_keys);
    else if (key == get_action_key(world.keybindings, "toggle map window"))
      toggle_window(&win_meta, &win_map);
    else if (key == get_action_key(world.keybindings, "toggle info window"))
      toggle_window(&win_meta, &win_info);
    else if (key == get_action_key(world.keybindings, "toggle log window"))
      toggle_window(&win_meta, &win_log);
    else if (key == get_action_key(world.keybindings, "cycle forwards"))
      cycle_active_window(&win_meta, 'n');
    else if (key == get_action_key(world.keybindings, "cycle backwards"))
      cycle_active_window(&win_meta, 'p');
    else if (key == get_action_key(world.keybindings, "shift forwards"))
      shift_active_window(&win_meta, 'f');
    else if (key == get_action_key(world.keybindings, "shift backwards"))
      shift_active_window(&win_meta, 'b');
    else if (key == get_action_key(world.keybindings, "grow horizontally"))
      growshrink_active_window(&win_meta, '*');
    else if (key == get_action_key(world.keybindings, "shrink horizontally"))
      growshrink_active_window(&win_meta, '_');
    else if (key == get_action_key(world.keybindings, "grow vertically"))
      growshrink_active_window(&win_meta, '+');
    else if (key == get_action_key(world.keybindings, "shrink vertically"))
      growshrink_active_window(&win_meta, '-');
    else if (key == get_action_key(world.keybindings, "save keys"))
      save_keybindings(&world);
    else if (key == get_action_key(world.keybindings, "keys nav up"))
      keyswin_move_selection (&world, 'u');
    else if (key == get_action_key(world.keybindings, "keys nav down"))
      keyswin_move_selection (&world, 'd');
    else if (key == get_action_key(world.keybindings, "keys mod"))
      keyswin_mod_key (&world, &win_meta);
    else if (key == get_action_key(world.keybindings, "save seed"))
      save_seed(seed);
    else if (key == get_action_key(world.keybindings, "map up"))
      map_scroll (&map, 'n');
    else if (key == get_action_key(world.keybindings, "map down"))
      map_scroll (&map, 's');
    else if (key == get_action_key(world.keybindings, "map right"))
      map_scroll (&map, 'e');
    else if (key == get_action_key(world.keybindings, "map left"))
      map_scroll (&map, 'w');
    else if (key == get_action_key(world.keybindings, "player down"))
      move_player(&world, 's');
    else if (key == get_action_key(world.keybindings, "player up"))
      move_player(&world, 'n');
    else if (key == get_action_key(world.keybindings, "player right"))
      move_player(&world, 'e');
    else if (key == get_action_key(world.keybindings, "player left"))
      move_player(&world, 'w');
    else if (key == get_action_key(world.keybindings, "wait") )
      player_wait (&world); }

  free(map.cells);
  for (key = 0; key <= world.keyswindata->max; key++)
    free(world.keybindings[key].name);
  free(world.keybindings);
  free(world.keyswindata);
  free(world.log);

  endwin();
  return 0; }
