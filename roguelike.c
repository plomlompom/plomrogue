#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "windows.h"

struct Map {
  int width;
  int height;
  int offset_x;
  int offset_y;
  int player_x;
  int player_y;
  char * cells; };

struct KeyBinding {
  char * name;
  int key; };

struct KeysWinData {
  int max;
  char edit;
  int select; };

struct World {
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata; };

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
           mvwaddch(win->curses_win, y, x, ' ');
           toggle = 1;
           continue; }
         else
           mvwaddch(win->curses_win, y, x, text[z]);
         if ('\n' == text[z+1]) {
           z++;
           toggle = 1; }
         else if (0 == text[z+1]) {
            toggle = 1;
            fin = 1; } }
       else
         mvwaddch(win->curses_win, y, x, ' '); } } }

void draw_text_from_bottom (struct Win * win) {
// Draw text in win->data from end/bottom to the top.
  char * text = (char *) win->data;
  char toggle = 0;
  int x, y, offset;
  int z = -1;
  for (y = 0; 0 == toggle; y++)         // Determine number of lines text would have in a window of
    for (x = 0; x < win->width; x++) {  // available width, but infinite height.
      z++;
      if ('\n' == text[z])              // Treat \n and \0 as control characters for incrementing y and
        break;                          // stopping the loop. Make sure they don't count as cell space
      if ('\n' == text[z+1]) {          // themselves.
        z++;
        break; }
      else if (0 == text[z+1]) {
        toggle = 1;
        break; } }
  z = -1;
  int start_y = 0;
  if (y < win->height) {          // Depending on what is bigger, determine start point in window or in text.
    start_y = win->height - y;
    for (y = 0; y < start_y; y++)
      for (x = 0; x < win->width; x++)
        mvwaddch(win->curses_win, y, x, ' '); }
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

void draw_map (struct Win * win) {
// Draw map determined by win->data Map struct into window. Respect offset.
  struct Map map = * (struct Map *) win->data;
  char * cells = map.cells;
  int width_map_av = map.width - map.offset_x;
  int height_map_av = map.height - map.offset_y;
  int x, y, z;
  for (y = 0; y < win->height; y++) {
    z = map.offset_x + (map.offset_y + y) * (map.width);
    for (x = 0; x < win->width; x++) {
      if (y < height_map_av && x < width_map_av) {
        if (z == (map.width * map.player_y) + map.player_x)
          mvwaddch(win->curses_win, y, x, '@');
        else
          mvwaddch(win->curses_win, y, x, cells[z]);
        z++; }
      else
        mvwaddch(win->curses_win, y, x, ' '); } } }

void draw_info (struct Win * win) {
// Draw info window by appending win->data integer value to "Turn: " display.
  int count = * (int *) win->data;
  char text[100];
  snprintf(text, 100, "Turn: %d", count);
  draw_with_linebreaks(win, text, 0); }

void toggle_window (struct WinMeta * win_meta, struct Win * win) {
// Toggle display of window win.
  if (0 != win->curses_win)
    suspend_window(win_meta, win);
  else
    append_window(win_meta, win); }

struct Map init_map () {
// Initialize map with some experimental start values.
  struct Map map;
  map.width = 128;
  map.height = 128;
  map.offset_x = 0;
  map.offset_y = 0;
  map.player_x = 2;
  map.player_y = 2;
  map.cells = malloc(map.width * map.height);
  int x, y;
  for (y = 0; y < map.height; y++)
    for (x = 0; x < map.width; x++)
      map.cells[(y * map.width) + x] = '.';
  map.cells[(5 * map.width) + 5] = 'X';
  map.cells[(3 * map.width) + 8] = 'X';
  map.cells[(8 * map.width) + 3] = 'X';
  return map; }

void update_info (struct Win * win) {
// Update info data by incrementing turn value.
  * (int *) win->data = * (int *) win->data + 1; }

void update_log (struct Win * win, char * text) {
// Update log with new text to be appended.
  char * new_text;
  int len_old = strlen(win->data);
  int len_new = strlen(text);
  int len_whole = len_old + len_new + 1;
  new_text = calloc(len_whole, sizeof(char));
  memcpy(new_text, win->data, len_old);
  memcpy(new_text + len_old, text, len_new);
  free(win->data);
  win->data = new_text; }

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

void draw_keys_window (struct Win * win) {
// Draw keybinding window.
  struct World * world = (struct World *) win->data;
  struct KeysWinData * keyswindata = (struct KeysWinData *) world->keyswindata;
  struct KeyBinding * keybindings = world->keybindings;
  int offset = 0;
  if (keyswindata->max >= win->height) {
    if (keyswindata->select > win->height / 2)
      if (keyswindata->select < (keyswindata->max - (win->height / 2)))
        offset = keyswindata->select - (win->height / 2);
      else
        offset = keyswindata->max - win->height + 1; }
  int keydescwidth = 9; // max length assured by get_keyname()
  char * keydesc = malloc(keydescwidth + 1);
  attr_t attri;
  int y, x;
  char * keyname;
  for (y = 0; 0 != keybindings[offset + y].name && y < win->height; y++) {
    attri = 0;
    if (y == keyswindata->select - offset) {
      attri = A_REVERSE;
      if (1 == keyswindata->edit)
        attri = attri | A_BLINK; }
    keyname = get_keyname(keybindings[y + offset].key);
    snprintf(keydesc, keydescwidth + 1, "%-9s", keyname);
    free(keyname);
    for (x = 0; x < win->width; x++)
      if (strlen(keydesc) > x)
        mvwaddch(win->curses_win, y, x, keydesc[x] | attri);
      else if (strlen(keydesc) < x && x < strlen(keybindings[y + offset].name) + strlen(keydesc) + 1)
        mvwaddch(win->curses_win, y, x, keybindings[y + offset].name[x - strlen(keydesc) - 1] | attri);
      else
        mvwaddch(win->curses_win, y, x, ' ' | attri); }
  free(keydesc);
  if (y < win->height)
    for (; y < win->height; y++)
      for (x = 0; x < win->width; x++)
        mvwaddch(win->curses_win, y, x, ' '); }

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
        linemax = c_count;
      c_count = 0;
      lines++; } }
  struct KeyBinding * keybindings = malloc(lines * sizeof(struct KeyBinding));
  fseek(file, 0, SEEK_SET);
  char * command = malloc(linemax);
  char commcount = 0;
  char * digits = malloc(3);
  char digicount = 0;
  int key, digimax;
  int keycount = 0;
  c = getc(file);
  while (EOF != c) {
    if ('\n' == c) {
      keybindings[keycount].name = calloc(commcount, sizeof(char));
      memcpy(keybindings[keycount].name, command, commcount);
      keybindings[keycount].key = key;
      keycount++;
      digicount = 0;
      commcount = 0; }
    else if (-1 != digicount)
      if (' ' == c) {
        key = 0;
        digimax = digicount - 1;
        while (digicount > 0) {
          digicount--;
          key = key + ((digits[digicount] - 48) * pow(10, digimax - digicount)); }
        digicount = -1; }
      else {
        digits[digicount] = c;
        digicount++; }
    else {
      command[commcount] = c;
      commcount++; }
    c = getc(file); }
  free(command);
  free(digits);
  fclose(file);
  struct KeysWinData * keyswindata = malloc(sizeof(struct KeysWinData));
  keyswindata->max = lines - 1;
  keyswindata->select = 0;
  keyswindata->edit = 0;
  world->keybindings = keybindings;
  world->keyswindata = keyswindata; }

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
  linemax = linemax + 6; // + 6 = + 3 digits + whitespace + newline + null byte
  char * line = malloc(linemax);
  for (i = 0; i <= keyswindata->max; i++) {
    snprintf(line, linemax, "%d %s\n", keybindings[i].key, keybindings[i].name);
    fwrite(line, sizeof(char), strlen(line), file); }
  free(line);
  fclose(file); }

int main () {
  struct World world;
  init_keybindings(&world);

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
  struct Map map = init_map();
  win_map.data = &map;

  struct Win win_info = init_window(&win_meta, "Info");
  win_info.draw = draw_info;
  win_info.data = malloc(sizeof(int));
  * (int *) win_info.data = 0;

  struct Win win_log = init_window(&win_meta, "Log");
  win_log.draw = draw_text_from_bottom;
  win_log.data = calloc(1, sizeof(char));
  update_log (&win_log, "Start!");

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
    else if (key == get_action_key(world.keybindings, "grow hor") && win_meta.active != 0)
      resize_window(&win_meta, '*');
    else if (key == get_action_key(world.keybindings, "shrink hor") && win_meta.active != 0)
      resize_window(&win_meta, '_');
    else if (key == get_action_key(world.keybindings, "grow ver") && win_meta.active != 0)
      resize_window(&win_meta, '+');
    else if (key == get_action_key(world.keybindings, "shrink ver") && win_meta.active != 0)
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
    else if (key == get_action_key(world.keybindings, "player down") && map.player_y < map.height - 1) {
      update_info (&win_info);
      update_log (&win_log, "\nYou move south.");
      map.player_y++; }
    else if (key == get_action_key(world.keybindings, "player up") && map.player_y > 0) {
      update_info (&win_info);
      update_log (&win_log, "\nYou move north.");
      map.player_y--; }
    else if (key == get_action_key(world.keybindings, "player right") && map.player_x < map.width - 1) {
      update_info (&win_info);
      update_log (&win_log, "\nYou move east.");
      map.player_x++; }
    else if (key == get_action_key(world.keybindings, "player left") && map.player_x > 0) {
      update_info (&win_info);
      update_log (&win_log, "\nYou move west.");
      map.player_x--; }
    else if (key == get_action_key(world.keybindings, "wait") ) {
      update_info (&win_info);
      update_log (&win_log, "\nYou wait."); } }

  endwin();
  return 0; }
