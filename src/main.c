#include "main.h"
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include "windows.h"
#include "draw_wins.h"
#include "keybindings.h"
#include "readwrite.h"
#include "map_objects.h"
#include "map.h"
#include "misc.h"

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
  player.hitpoints = 5;
  world.player = &player;
  world.monster = 0;
  world.item = 0;

  // For interactive mode, try to load world state from savefile.
  FILE * file;
  if (1 == world.interactive && 0 == access("savefile", F_OK)) {
    file = fopen("savefile", "r");
    world.seed = read_uint32_bigendian(file);
    world.turn = read_uint32_bigendian(file);
    player.pos.y = read_uint16_bigendian(file) - 1;
    player.pos.x = read_uint16_bigendian(file) - 1;
    player.hitpoints = fgetc(file);
    read_map_objects (&world.monster, file, sizeof(struct Monster), read_map_objects_monsterdata);
    read_map_objects (&world.item,    file, sizeof(struct Item),    readwrite_map_objects_dummy);
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
    player.pos = find_passable_pos(&map);
    unsigned char n_monsters = rrand(0, 0) % 16;
    unsigned char n_items    = rrand(0, 0) % 48;
    build_map_objects (&world.monster, n_monsters, sizeof(struct Monster), build_map_objects_monsterdata, &map);
    build_map_objects (&world.item,    n_items,    sizeof(struct Item),    build_map_objects_itemdata,    &map); }

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
  win_info.frame.size.y = 2;
  win_log.frame.size.y = win_meta.pad.size.y - (2 + win_info.frame.size.y);
  toggle_window(&win_meta, &win_keys);
  toggle_window(&win_meta, &win_map);
  toggle_window(&win_meta, &win_info);
  toggle_window(&win_meta, &win_log);

  // Replay mode.
  int key;
  unsigned char quit_called = 0;
  unsigned char await_actions = 1;
  if (0 == world.interactive) {
    int action;
    while (1) {
      if (start_turn == world.turn)
        start_turn = 0;
      if (0 == start_turn) {
        draw_all_wins (&win_meta);
        key = getch(); }
      if (1 == await_actions &&
          (world.turn < start_turn || key == get_action_key(world.keybindings, "wait / next turn")) ) {
        action = getc(file);
        if (EOF == action) {
          start_turn = 0;
          await_actions = 0; }
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
      if (1 == await_actions && 0 == player.hitpoints)
        await_actions = 0;
      draw_all_wins (&win_meta);
      key = getch();
      if      (1 == await_actions && key == get_action_key(world.keybindings, "player up"))
        move_player(&world, NORTH);
      else if (1 == await_actions && key == get_action_key(world.keybindings, "player right"))
        move_player(&world, EAST);
      else if (1 == await_actions && key == get_action_key(world.keybindings, "player down"))
        move_player(&world, SOUTH);
      else if (1 == await_actions && key == get_action_key(world.keybindings, "player left"))
        move_player(&world, WEST);
      else if (1 == await_actions && key == get_action_key(world.keybindings, "wait / next turn"))
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
