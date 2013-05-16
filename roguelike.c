#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

struct World {
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata;
  int turn;
  char * log;
  struct Map * map;
  struct Monster * monster;
  struct Player * player; };

struct KeyBinding {
  char * name;
  int key; };

struct KeysWinData {
  int max;
  char edit;
  int select; };

struct Map {
  int width;
  int height;
  int offset_x;
  int offset_y;
  char * cells; };

struct Player {
  int y;
  int x; };

struct Monster {
  int y;
  int x; };

void draw_with_linebreaks (struct Win *, char *, int);
void draw_text_from_bottom (struct Win *, char *);
void draw_log (struct Win *);
void draw_map (struct Win *);
void draw_info (struct Win *);
void draw_keys_window (struct Win *);
void toggle_window (struct WinMeta *, struct Win *);
void init_keybindings(struct World *);
struct Map init_map ();
void next_turn (struct World *);
void update_log (struct World *, char *);
void save_keybindings(struct World *);
int get_action_key (struct KeyBinding *, char *);
char * get_keyname(int);
char is_passable (struct World *, int, int);
void move_player (struct World *, char);
void player_wait(struct World *);

void draw_with_linebreaks (struct Win * win, char * text, int start_y) {
// Write text into window content space. Start on row start_y. Fill unused rows with whitespace.
  int x, y;
  char toggle;
  char fin = 0;
  int z = -1;
  for (y = start_y; y < win->height; y++) {
    if (0 == fin)
      toggle = 0;
    for (x = 0; x < win->width; x++) {
       if (0 == toggle) {
         z++;
         if ('\n' == text[z]) {
           toggle = 1;
           continue; }
         else
           mvwaddch(win->curses, y, x, text[z]);
         if ('\n' == text[z+1]) {
           z++;
           toggle = 1; }
         else if (0 == text[z+1]) {
            toggle = 1;
            fin = 1; } } } } }

void draw_text_from_bottom (struct Win * win, char * text) {
// Draw text from end/bottom to the top.
  char toggle = 0;
  int x, y, offset;
  int z = -1;
  for (y = 0; 0 == toggle; y++)                           // Determine number of lines text would have in
    for (x = 0; x < win->width; x++) {                    // a window of available width, but infinite height.
      z++;
      if ('\n' == text[z])            // Treat \n and \0 as control characters for incrementing y and stopping
        break;                        // the loop. Make sure they don't count as cell space themselves.
      if ('\n' == text[z+1]) {
        z++;
        break; }
      else if (0 == text[z+1]) {
        toggle = 1;
        break; } }
  z = -1;
  int start_y = 0;
  if (y < win->height)             // Depending on what is bigger, determine start point in window or in text.
    start_y = win->height - y;
  else if (y > win->height) {
    offset = y - win->height;
    for (y = 0; y < offset; y++)
      for (x = 0; x < win->width; x++) {
        z++;
        if ('\n' == text[z])
          break;
        if ('\n' == text[z+1]) {
          z++;
          break; } }
    text = text + (sizeof(char) * (z + 1)); }
  draw_with_linebreaks(win, text, start_y); }

void draw_log (struct Win * win) {
// Draw log text from world struct in win->data from bottom to top.
  struct World * world = (struct World *) win->data;
  draw_text_from_bottom(win, world->log); }

void draw_map (struct Win * win) {
// Draw map determined by win->data Map struct into window. Respect offset.
  struct World * world = (struct World *) win->data;
  struct Map * map = world->map;
  struct Player * player = world->player;
  struct Monster * monster = world->monster;
  char * cells = map->cells;
  int width_map_av = map->width - map->offset_x;
  int height_map_av = map->height - map->offset_y;
  int x, y, z;
  for (y = 0; y < win->height; y++) {
    z = map->offset_x + (map->offset_y + y) * (map->width);
    for (x = 0; x < win->width; x++) {
      if (y < height_map_av && x < width_map_av) {
        if (z == (map->width * player->y) + player->x)
          mvwaddch(win->curses, y, x, '@');
        else if (z == (map->width * monster->y) + monster->x)
          mvwaddch(win->curses, y, x, 'M');
        else
          mvwaddch(win->curses, y, x, cells[z]);
        z++; } } } }

void draw_info (struct Win * win) {
// Draw info window by appending win->data integer value to "Turn: " display.
  struct World * world = (struct World *) win->data;
  int count = world->turn;
  char text[100];
  snprintf(text, 100, "Turn: %d", count);
  draw_with_linebreaks(win, text, 0); }

void draw_keys_window (struct Win * win) {
// Draw keybinding window.
  struct World * world = (struct World *) win->data;
  struct KeysWinData * keyswindata = (struct KeysWinData *) world->keyswindata;
  struct KeyBinding * keybindings = world->keybindings;
  int offset = 0;
  if (keyswindata->max >= win->height) {
    if (keyswindata->select > win->height / 2) {
      if (keyswindata->select < (keyswindata->max - (win->height / 2)))
        offset = keyswindata->select - (win->height / 2);
      else
        offset = keyswindata->max - win->height + 1; } }
  int keydescwidth = 9 + 1; // max length assured by get_keyname() + \0
  char * keydesc = malloc(keydescwidth);
  attr_t attri;
  int y, x;
  char * keyname;
  for (y = 0; y <= keyswindata->max && y < win->height; y++) {
    attri = 0;
    if (y == keyswindata->select - offset) {
      attri = A_REVERSE;
      if (1 == keyswindata->edit)
        attri = attri | A_BLINK; }
    keyname = get_keyname(keybindings[y + offset].key);
    snprintf(keydesc, keydescwidth, "%-9s", keyname);
    free(keyname);
    for (x = 0; x < win->width; x++)
      if (x < strlen(keydesc))
        mvwaddch(win->curses, y, x, keydesc[x] | attri);
      else if (strlen(keydesc) < x && x < strlen(keybindings[y + offset].name) + strlen(keydesc) + 1)
        mvwaddch(win->curses, y, x, keybindings[y + offset].name[x - strlen(keydesc) - 1] | attri);
      else
        mvwaddch(win->curses, y, x, ' ' | attri); }
  free(keydesc); }

void toggle_window (struct WinMeta * win_meta, struct Win * win) {
// Toggle display of window win.
  if (0 != win->curses)
    suspend_window(win_meta, win);
  else
    append_window(win_meta, win); }

void init_keybindings(struct World * world) {
// Initialize keybindings from file "keybindings".
  FILE * file = fopen("keybindings", "r");
  int lines = 0;
  int c = 0;
  int linemax = 0;
  int c_count = 0;
  while (EOF != c) {
    c_count++;
    c = getc(file);
    if ('\n' == c) {
      if (c_count > linemax)
        linemax = c_count + 1;
      c_count = 0;
      lines++; } }
  struct KeyBinding * keybindings = malloc(lines * sizeof(struct KeyBinding));
  fseek(file, 0, SEEK_SET);
  char * command = malloc(linemax);
  int commcount = 0;
  char * cmdptr;
  while (fgets(command, linemax, file)) {
    keybindings[commcount].key = atoi(command);
    cmdptr = strchr(command, ' ') + 1;
    keybindings[commcount].name = malloc(strlen(cmdptr));
    memcpy(keybindings[commcount].name, cmdptr, strlen(cmdptr) - 1);
    keybindings[commcount].name[strlen(cmdptr) - 1] = '\0';
    commcount++; }
  free(command);
  fclose(file);
  struct KeysWinData * keyswindata = malloc(sizeof(struct KeysWinData));
  keyswindata->max = lines - 1;
  keyswindata->select = 0;
  keyswindata->edit = 0;
  world->keybindings = keybindings;
  world->keyswindata = keyswindata; }

struct Map init_map () {
// Initialize map with some experimental start values.
  struct Map map;
  map.width = 96;
  map.height = 32;
  map.offset_x = 0;
  map.offset_y = 0;
  map.cells = malloc(map.width * map.height);
  int x, y, ran;
  char terrain;
  for (y = 0; y < map.height; y++)
    for (x = 0; x < map.width; x++) {
      terrain = '.';
      ran = rand();
      if (   0 == ran % ((x*x) / 3 + 1)
          || 0 == ran % ((y*y) / 3 + 1)
          || 0 == ran % ((map.width - x - 1) * (map.width - x - 1) / 3 + 1)
          || 0 == ran %((map.height - y - 1) * (map.height - y - 1) / 3 + 1))
        terrain = ' ';
      map.cells[(y * map.width) + x] = terrain; }
  return map; }

void next_turn (struct World * world) {
// Increment turn and move enemy.
  world->turn++;
  char d = rand() % 5;
  char ty = world->monster->y;
  char tx = world->monster->x;
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
  int len_old = strlen(world->log);
  int len_new = strlen(text);
  int len_whole = len_old + len_new + 1;
  new_text = calloc(len_whole, sizeof(char));
  memcpy(new_text, world->log, len_old);
  memcpy(new_text + len_old, text, len_new);
  free(world->log);
  world->log = new_text; }

void save_keybindings(struct World * world) {
// Write keybindings to keybindings file.
  struct KeysWinData * keyswindata = (struct KeysWinData *) world->keyswindata;
  struct KeyBinding * keybindings = world->keybindings;
  FILE * file = fopen("keybindings", "w");
  int linemax = 0;
  int i;
  for (i = 0; i <= keyswindata->max; i++)
    if (strlen(keybindings[i].name) > linemax)
      linemax = strlen(keybindings[i].name);
  linemax = linemax + 6;                                // + 6 = + 3 digits + whitespace + newline + null byte
  char * line = malloc(linemax);
  for (i = 0; i <= keyswindata->max; i++) {
    snprintf(line, linemax, "%d %s\n", keybindings[i].key, keybindings[i].name);
    fwrite(line, sizeof(char), strlen(line), file); }
  free(line);
  fclose(file); }

int get_action_key (struct KeyBinding * keybindings, char * name) {
// Return key matching name in keybindings.
  int i = 0;
  while (strcmp(keybindings[i].name, name) )
    i++;
  return keybindings[i].key; }

char * get_keyname(int keycode) {
// Translate some keycodes to readable names of max 9 chars.
  char * keyname;
  keyname = malloc(15);
  if (32 < keycode && keycode < 127)
    sprintf(keyname, "%c", keycode);
  else if (keycode == 9)
    sprintf(keyname, "TAB");
  else if (keycode == 10)
    sprintf(keyname, "RETURN");
  else if (keycode == 27)
    sprintf(keyname, "ESCAPE");
  else if (keycode == 32)
    sprintf(keyname, "SPACE");
  else if (keycode == KEY_UP)
    sprintf(keyname, "UP");
  else if (keycode == KEY_DOWN)
    sprintf(keyname, "DOWN");
  else if (keycode == KEY_LEFT)
    sprintf(keyname, "LEFT");
  else if (keycode == KEY_RIGHT)
    sprintf(keyname, "RIGHT");
  else if (keycode == KEY_HOME)
    sprintf(keyname, "HOME");
  else if (keycode == KEY_BACKSPACE)
    sprintf(keyname, "BACKSPACE");
  else if (keycode >= KEY_F0 && keycode <= KEY_F(63)) {
    int f = keycode - KEY_F0;
    sprintf(keyname, "F%d", f); }
  else if (keycode == KEY_DC)
    sprintf(keyname, "DELETE");
  else if (keycode == KEY_IC)
    sprintf(keyname, "INSERT");
  else if (keycode == KEY_NPAGE)
    sprintf(keyname, "NEXT PAGE");
  else if (keycode == KEY_PPAGE)
    sprintf(keyname, "PREV PAGE");
  else if (keycode == KEY_END)
    sprintf(keyname, "END");
  else
    sprintf(keyname, "(unknown)");
  return keyname;  }

char is_passable (struct World * world, int x, int y) {
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
  char ty = world->player->y;
  char tx = world->player->x;
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

void player_wait(struct World * world) {
// Make player wait one turn.
  next_turn (world);
  update_log (world, "\nYou wait."); }

int main () {
  struct World world;
  init_keybindings(&world);
  world.turn = 0;
  world.log = calloc(1, sizeof(char));
  update_log (&world, "Start!");
  struct Map map = init_map();
  world.map = &map;
  struct Player player;
  player.y = 16;
  player.x = 16;
  world.player = &player;
  struct Monster monster;
  monster.y = 16;
  monster.x = 80;
  world.monster = &monster;

  WINDOW * screen = initscr();
  noecho();
  curs_set(0);
  keypad(screen, TRUE);
  raw();
  struct WinMeta win_meta = init_win_meta(screen);

  struct Win win_keys = init_window(&win_meta, "Keys");
  win_keys.draw = draw_keys_window;
  win_keys.data = &world;
  struct Win win_map = init_window(&win_meta, "Map");
  win_map.draw = draw_map;
  win_map.data = &world;
  struct Win win_info = init_window(&win_meta, "Info");
  win_info.draw = draw_info;
  win_info.data = &world;
  struct Win win_log = init_window(&win_meta, "Log");
  win_log.draw = draw_log;
  win_log.data = &world;

  int key;
  while (1) {
    draw_all_windows (&win_meta);
    key = getch();
    if      (key == get_action_key(world.keybindings, "quit"))
      break;
    else if (key == get_action_key(world.keybindings, "scroll pad right"))
      win_meta.pad_offset++;
    else if (key == get_action_key(world.keybindings, "scroll pad left") && win_meta.pad_offset > 0)
      win_meta.pad_offset--;
    else if (key == get_action_key(world.keybindings, "toggle keys window"))
      toggle_window(&win_meta, &win_keys);
    else if (key == get_action_key(world.keybindings, "toggle map window"))
      toggle_window(&win_meta, &win_map);
    else if (key == get_action_key(world.keybindings, "toggle info window"))
      toggle_window(&win_meta, &win_info);
    else if (key == get_action_key(world.keybindings, "toggle log window"))
      toggle_window(&win_meta, &win_log);
    else if (key == get_action_key(world.keybindings, "cycle forwards") && win_meta.active != 0)
      cycle_active_window(&win_meta, 'n');
    else if (key == get_action_key(world.keybindings, "cycle backwards") && win_meta.active != 0)
      cycle_active_window(&win_meta, 'p');
    else if (key == get_action_key(world.keybindings, "shift forwards")  && win_meta.active != 0)
      shift_window(&win_meta, 'f');
    else if (key == get_action_key(world.keybindings, "shift backwards") && win_meta.active != 0)
      shift_window(&win_meta, 'b');
    else if (key == get_action_key(world.keybindings, "grow horizontally") && win_meta.active != 0)
      resize_window(&win_meta, '*');
    else if (key == get_action_key(world.keybindings, "shrink horizontally") && win_meta.active != 0)
      resize_window(&win_meta, '_');
    else if (key == get_action_key(world.keybindings, "grow vertically") && win_meta.active != 0)
      resize_window(&win_meta, '+');
    else if (key == get_action_key(world.keybindings, "shrink vertically") && win_meta.active != 0)
      resize_window(&win_meta, '-');
    else if (key == get_action_key(world.keybindings, "save keys"))
      save_keybindings(&world);
    else if (key == get_action_key(world.keybindings, "keys nav up") && world.keyswindata->select > 0)
      world.keyswindata->select--;
    else if (key == get_action_key(world.keybindings, "keys nav down") && world.keyswindata->select < world.keyswindata->max)
      world.keyswindata->select++;
    else if (key == get_action_key(world.keybindings, "keys mod")) {
      world.keyswindata->edit = 1;
      draw_all_windows (&win_meta);
      key = getch();
      if (key < 1000) // ensure maximum of three digits in key code field
        world.keybindings[world.keyswindata->select].key = key;
      world.keyswindata->edit = 0; }
    else if (key == get_action_key(world.keybindings, "map up") && map.offset_y > 0)
      map.offset_y--;
    else if (key == get_action_key(world.keybindings, "map down"))
      map.offset_y++;
    else if (key == get_action_key(world.keybindings, "map right"))
      map.offset_x++;
    else if (key == get_action_key(world.keybindings, "map left") && map.offset_x > 0)
      map.offset_x--;
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
