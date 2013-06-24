#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "windows.h"
#include "draw_wins.h"
#include "roguelike.h"
#include "keybindings.h"
#include "readwrite.h"

#define NORTH 1
#define EAST 2
#define SOUTH 3
#define WEST 4

uint16_t rrand(char use_seed, uint32_t new_seed) {
// Pseudo-random number generator (LGC algorithm). Use instead of rand() to ensure portable predictability.
  static uint32_t seed;
  if (0 != use_seed)
    seed = new_seed;
  seed = ((seed * 1103515245) + 12345) % 2147483648;   // Values as recommended by POSIX.1-2001 (see rand(3)).
  return (seed / 65536); }                         // Ignore least significant 16 bits (they are less random).

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

void record_action (char action) {
// Append action to game record file.
  FILE * file = fopen("record", "a");
  fputc(action, file);
  fclose(file); }

void next_turn (struct World * world) {
// Increment turn and move enemy.
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
  write_uint16_bigendian(world->player->pos.y, file);
  write_uint16_bigendian(world->player->pos.x, file);
  write_uint16_bigendian(world->monster->pos.y, file);
  write_uint16_bigendian(world->monster->pos.x, file);
  write_uint16_bigendian(world->monster->next->pos.y, file);
  write_uint16_bigendian(world->monster->next->pos.x, file);
  write_uint16_bigendian(world->monster->next->next->pos.y, file);
  write_uint16_bigendian(world->monster->next->next->pos.x, file);
  fclose(file); }

char is_passable (struct Map * map, uint16_t y, uint16_t x) {
// Check if coordinate on (or beyond) map is accessible to movement.
  char passable = 0;
  if (0 <= x && x < map->size.x && 0 <= y && y < map->size.y)
    if ('.' == map->cells[y * map->size.x + x])
      passable = 1;
  return passable; }

struct yx_uint16 mv_yx_in_dir (char d, struct yx_uint16 yx) {
// Return yx coordinates one step to the direction d of yx.
  if      (d == NORTH) yx.y--;
  else if (d == EAST)  yx.x++;
  else if (d == SOUTH) yx.y++;
  else if (d == WEST)  yx.x--;
  return yx; }

void move_monster (struct World * world, struct Monster * monster) {
// Move monster in random direction, trigger fighting when hindered by player/monster.
  char d = rrand(0, 0) % 5;
  struct yx_uint16 t = mv_yx_in_dir (d, monster->pos);
  if (yx_uint16_cmp (t, world->player->pos)) {
    update_log (world, "\nThe monster hits you.");
    return; }
  char met_monster = 0;
  struct Monster * other_monster;
  for (other_monster = world->monster; other_monster != 0; other_monster = other_monster->next) {
    if (other_monster == monster)
      continue;
    if (yx_uint16_cmp (t, other_monster->pos)) {
      met_monster = 1;
      break; } }
  if (met_monster)
    update_log (world, "\nMonster hits monster.");
  else if (0 == met_monster && is_passable(world->map, t.y, t.x))
    monster->pos = t; }

void move_player (struct World * world, char d) {
// Move player in direction d, increment turn counter and update log.
  static char prev = 0;
  char success = 0;
  char * dir;
  struct yx_uint16 t = mv_yx_in_dir (d, world->player->pos);
  struct Monster * monster;
  for (monster = world->monster; monster != 0; monster = monster->next)
    if (yx_uint16_cmp (t, monster->pos)) {
      success = 2;
      break; }
  if (2 != success && is_passable(world->map, t.y, t.x)) {
    success = 1;
    world->player->pos = t; }
  if (success * d == prev)
    update_log (world, ".");
  else {
    if (2 == success)
      update_log (world, "\nYou hit the monster.");
    else {
      if      (NORTH == d) dir = "north";
      else if (EAST  == d) dir = "east" ;
      else if (SOUTH == d) dir = "south";
      else if (WEST  == d) dir = "west" ;
      char * msg = calloc(25, sizeof(char));
      char * msg_content = "You fail to move";
      if (1 == success)
        msg_content = "You move";
      sprintf(msg, "\n%s %s.", msg_content, dir);
      update_log (world, msg);
      free(msg); } }
  prev = success * d;
  if (1 == world->interactive)
    record_action(d);
  next_turn (world); }

void player_wait (struct World * world) {
// Make player wait one turn.
  if (1 == world->interactive)
    record_action(0);
  next_turn (world);
  update_log (world, "\nYou wait."); }

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
    uint16_t height = win_meta->active->frame.size.y;
    uint16_t width = win_meta->active->frame.size.x;
    if      (change == '-')
      height--;
    else if (change == '+')
      height++;
    else if (change == '_')
      width--;
    else if (change == '*')
      width++;
    resize_active_win (win_meta, height, width); } }

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

  // Initialize log, player and monsters.
  world.log = calloc(1, sizeof(char));
  update_log (&world, " ");
  struct Player player;
  world.player = &player;
  struct Monster monster1;
  struct Monster monster2;
  struct Monster monster3;
  world.monster = &monster1;
  monster1.next = &monster2;
  monster2.next = &monster3;
  monster3.next = 0;
  monster1.name = 'A';
  monster2.name = 'B';
  monster3.name = 'C';

  // For interactive mode, try to load world state from savefile.
  FILE * file;
  if (1 == world.interactive && 0 == access("savefile", F_OK)) {
    file = fopen("savefile", "r");
    world.seed = read_uint32_bigendian(file);
    world.turn = read_uint32_bigendian(file);
    player.pos.y = read_uint16_bigendian(file);
    player.pos.x = read_uint16_bigendian(file);
    monster1.pos.y = read_uint16_bigendian(file);
    monster1.pos.x = read_uint16_bigendian(file);
    monster2.pos.y = read_uint16_bigendian(file);
    monster2.pos.x = read_uint16_bigendian(file);
    monster3.pos.y = read_uint16_bigendian(file);
    monster3.pos.x = read_uint16_bigendian(file);
    fclose(file); }

  // For non-interactive mode, try to load world state from frecord file.
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
    for (player.pos.y = player.pos.x = 0; 0 == is_passable(&map, player.pos.y, player.pos.x);) {
      player.pos.y = rrand(0, 0) % map.size.y;
      player.pos.x = rrand(0, 0) % map.size.x; }
    struct Monster * monster;
    for (monster = world.monster; monster != 0; monster = monster->next)
      for (monster->pos.y = monster->pos.x = 0; 0 == is_passable(&map, monster->pos.y, monster->pos.x);) {
        monster->pos.y = rrand(0, 0) % map.size.y;
        monster->pos.x = rrand(0, 0) % map.size.x; } }

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
