#include "roguelike.h"
#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "windows.h"
#include "draw_wins.h"
#include "keybindings.h"
#include "readwrite.h"
#include "actors.h"

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

struct Map init_map () {
// Initialize map with some experimental start values.
  struct Map map;
  map.size.x = 64;
  map.size.y = 64;
  map.offset.x = 0;
  map.offset.y = 0;
  uint32_t size = map.size.x * map.size.y;
  map.cells = malloc(size);
  uint16_t y, x;
  for (y = 0; y < map.size.y; y++)
    for (x = 0; x < map.size.x; x++)
      map.cells[(y * map.size.x) + x] = '~';
  map.cells[size / 2 + (map.size.x / 2)] = '.';
  uint32_t repeats, root, curpos;
  for (root = 0; root * root * root < size; root++);
  for (repeats = 0; repeats < size * root; repeats++) {
    y = rrand(0, 0) % map.size.y;
    x = rrand(0, 0) % map.size.x;
    curpos = y * map.size.x + x;
    if ('~' == map.cells[curpos] &&
        (   (curpos >= map.size.x && '.' == map.cells[curpos - map.size.x])
         || (curpos < map.size.x * (map.size.y-1) && '.' == map.cells[curpos + map.size.x])
         || (curpos > 0 && curpos % map.size.x != 0 && '.' == map.cells[curpos-1])
         || (curpos < (map.size.x * map.size.y) && (curpos+1) % map.size.x != 0 && '.' == map.cells[curpos+1])))
      map.cells[y * map.size.x + x] = '.'; }
  return map; }

void map_scroll (struct Map * map, char dir) {
// Scroll map into direction dir if possible by changing the offset.
  if      (NORTH == dir && map->offset.y > 0) map->offset.y--;
  else if (SOUTH == dir)                      map->offset.y++;
  else if (WEST  == dir && map->offset.x > 0) map->offset.x--;
  else if (EAST  == dir)                      map->offset.x++; }

void turn_over (struct World * world, char action) {
// Record action in game record file, increment turn and move enemy.
  if (1 == world->interactive) {
    FILE * file = fopen("record", "a");
    fputc(action, file);
    fclose(file); }
  world->turn++;
  rrand(1, world->seed * world->turn);
  struct Monster * monster;
  for (monster = world->monster; monster != 0; monster = monster->next)
    move_monster(world, monster); }

void save_game(struct World * world) {
// Save game data to game file.
  FILE * file = fopen("savefile", "w");
  write_uint32_bigendian(world->seed, file);
  write_uint32_bigendian(world->turn, file);
  write_uint16_bigendian(world->player->pos.y + 1, file);
  write_uint16_bigendian(world->player->pos.x + 1, file);
  struct Monster * monster;
  for (monster = world->monster; monster != 0; monster = monster->next) {
    write_uint16_bigendian(monster->pos.y + 1, file);
    write_uint16_bigendian(monster->pos.x + 1, file); }
  write_uint16_bigendian(0, file);
  struct Item * item;
  for (item = world->item; item != 0; item = item->next) {
    write_uint16_bigendian(item->pos.y + 1, file);
    write_uint16_bigendian(item->pos.x + 1, file); }
  write_uint16_bigendian(0, file);
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
    map_scroll (world->map, NORTH);
  else if (key == get_action_key(world->keybindings, "map down"))
    map_scroll (world->map, SOUTH);
  else if (key == get_action_key(world->keybindings, "map right"))
    map_scroll (world->map, EAST);
  else if (key == get_action_key(world->keybindings, "map left"))
    map_scroll (world->map, WEST);
  return 0; }

int main (int argc, char *argv[]) {
  struct World world;

  // Read in startup options (i.e. replay option and replay start turn).
  int opt;
  uint32_t start_turn;
  world.interactive = 1;
  while ((opt = getopt(argc, argv, "s::")) != -1) {
    switch (opt) {
      case 's':
        world.interactive = 0;
        start_turn = 0;
        if (optarg)
          start_turn = atoi(optarg);
        break;
      default:
        exit(EXIT_FAILURE); } }

  // Initialize log, player, monsters and items.
  world.log = calloc(1, sizeof(char));
  update_log (&world, " ");
  struct Player player;
  world.player = &player;
  struct Monster * monster;
  struct Item * item;
  uint16_t test;
  char start;

  // For interactive mode, try to load world state from savefile.
  FILE * file;
  if (1 == world.interactive && 0 == access("savefile", F_OK)) {
    file = fopen("savefile", "r");
    world.seed = read_uint32_bigendian(file);
    world.turn = read_uint32_bigendian(file);
    player.pos.y = read_uint16_bigendian(file) - 1;
    player.pos.x = read_uint16_bigendian(file) - 1;
    start = 1;
    world.monster = 0;
    while (1) {
      test = read_uint16_bigendian(file);
      if (0 == test)
        break;
      if (start) {
        monster = malloc(sizeof(struct Monster));
        world.monster = monster;
        start = 0; }
      else {
        monster->next = malloc(sizeof(struct Monster));
        monster = monster->next; }
      monster->name = 'M';
      monster->pos.y = test - 1;
      monster->pos.x = read_uint16_bigendian(file) - 1; }
    if (!start)
      monster->next = 0;
    start = 1;
    world.item = 0;
    while (1) {
      test = read_uint16_bigendian(file);
      if (0 == test)
        break;
      if (start) {
        item = malloc(sizeof(struct Item));
        world.item = item;
        start = 0; }
      else {
        item->next = malloc(sizeof(struct Item));
        item = item->next; }
      item->name = '#';
      item->pos.y = test - 1;
      item->pos.x = read_uint16_bigendian(file) - 1; }
    if (!start)
      item->next = 0;
    fclose(file); }

  // For non-interactive mode, try to load world state from record file.
  else {
    world.turn = 1;
    if (0 == world.interactive) {
      file = fopen("record", "r");
      world.seed = read_uint32_bigendian(file); }

    // For interactive-mode in newly started world, generate a start seed from the current time.
    else {
      file = fopen("record", "w");
      world.seed = time(NULL);
      write_uint32_bigendian(world.seed, file);
      fclose(file); } }

  // Generate map from seed and, if newly generated world, start positions of actors.
  rrand(1, world.seed);
  struct Map map = init_map();
  world.map = &map;
  if (1 == world.turn) {
    for (player.pos.y = player.pos.x = 0; 0 == is_passable(&map, player.pos);) {
      player.pos.y = rrand(0, 0) % map.size.y;
      player.pos.x = rrand(0, 0) % map.size.x; }
    unsigned char n_monsters = rrand(0, 0) % 16;
    unsigned char n_items    = rrand(0, 0) % 48;
    unsigned char i;
    start = 1;
    world.monster = 0;
    for (i = 0; i < n_monsters; i++) {
      if (start) {
        monster = malloc(sizeof(struct Monster));
        world.monster = monster;
        start = 0; }
      else {
        monster->next = malloc(sizeof(struct Monster));
        monster = monster->next; }
      for (monster->pos.y = monster->pos.x = 0; 0 == is_passable(&map, monster->pos);) {
        monster->pos.y = rrand(0, 0) % map.size.y;
        monster->pos.x = rrand(0, 0) % map.size.x; }
      monster->name = 'M'; }
    if (!start)
      monster->next = 0;
    start = 1;
    world.item = 0;
    for (i = 0; i < n_items; i++) {
      if (start) {
        item = malloc(sizeof(struct Item));
        world.item = item;
        start = 0; }
      else {
        item->next = malloc(sizeof(struct Item));
        item = item->next; }
      for (item->pos.y = item->pos.x = 0; 0 == is_passable(&map, item->pos);) {
        item->pos.y = rrand(0, 0) % map.size.y;
        item->pos.x = rrand(0, 0) % map.size.x; }
      item->name = '#'; }
    if (!start)
      item->next = 0; }

  // Initialize window system and windows.
  WINDOW * screen = initscr();
  noecho();
  curs_set(0);
  keypad(screen, TRUE);
  raw();
  init_keybindings(&world);
  struct WinMeta win_meta = init_win_meta(screen);
  struct Win win_keys = init_win(&win_meta, "Keys", &world, draw_keys_win);
  struct Win win_map = init_win(&win_meta, "Map", &world, draw_map_win);
  struct Win win_info = init_win(&win_meta, "Info", &world, draw_info_win);
  struct Win win_log = init_win(&win_meta, "Log", &world, draw_log_win);
  win_keys.frame.size.x = 29;
  win_map.frame.size.x = win_meta.pad.size.x - win_keys.frame.size.x - win_log.frame.size.x - 2;
  win_info.frame.size.y = 1;
  win_log.frame.size.y = win_meta.pad.size.y - 3;
  toggle_window(&win_meta, &win_keys);
  toggle_window(&win_meta, &win_map);
  toggle_window(&win_meta, &win_info);
  toggle_window(&win_meta, &win_log);

  // Replay mode.
  int key;
  unsigned char quit_called = 0;
  if (0 == world.interactive) {
    unsigned char still_reading_file = 1;
    int action;
    while (1) {
      if (start_turn == world.turn)
        start_turn = 0;
      if (0 == start_turn) {
        draw_all_wins (&win_meta);
        key = getch(); }
      if (1 == still_reading_file &&
          (world.turn < start_turn || key == get_action_key(world.keybindings, "wait / next turn")) ) {
        action = getc(file);
        if (EOF == action) {
          start_turn = 0;
          still_reading_file = 0; }
        else if (0 == action)
          player_wait (&world);
        else if (NORTH == action)
          move_player(&world, NORTH);
        else if (EAST  == action)
          move_player(&world, EAST);
        else if (SOUTH == action)
          move_player(&world, SOUTH);
        else if (WEST == action)
          move_player(&world, WEST); }
      else
        quit_called = meta_keys(key, &world, &win_meta, &win_keys, &win_map, &win_info, &win_log);
        if (1 == quit_called)
          break; } }

  // Interactive mode.
  else {
    uint32_t last_turn = 0;
    while (1) {
      if (last_turn != world.turn) {
        save_game(&world);
        last_turn = world.turn; }
      draw_all_wins (&win_meta);
      key = getch();
      if      (key == get_action_key(world.keybindings, "player up"))
        move_player(&world, NORTH);
      else if (key == get_action_key(world.keybindings, "player right"))
        move_player(&world, EAST);
      else if (key == get_action_key(world.keybindings, "player down"))
        move_player(&world, SOUTH);
      else if (key == get_action_key(world.keybindings, "player left"))
        move_player(&world, WEST);
      else if (key == get_action_key(world.keybindings, "wait / next turn"))
        player_wait (&world);
      else
        quit_called = meta_keys(key, &world, &win_meta, &win_keys, &win_map, &win_info, &win_log);
        if (1 == quit_called)
          break; } }

  // Clean up and exit.
  free(map.cells);
  for (key = 0; key <= world.keyswindata->max; key++)
    free(world.keybindings[key].name);
  free(world.keybindings);
  free(world.keyswindata);
  free(world.log);
  endwin();
  return 0; }
