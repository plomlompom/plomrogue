#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

struct Map {
  int width;
  int height;
  int offset_x;
  int offset_y;
  int player_x;
  int player_y;
  char * cells; };

void draw_with_linebreaks (struct Win * win, char * text, int start_y) {
// Write text into window content space. Start on row start_y. Fill unused rows with whitespace.
  int x, y;
  char toggle;
  int height_av = win->height - 1;
  int width_av = win->width - 1;
  char fin = 0;
  int z = -1;
  for (y = start_y; y < height_av; y++) {
    if (0 == fin)
      toggle = 0;
    for (x = 0; x < width_av; x++) {
       if (0 == toggle) {
         z++;
         if ('\n' == text[z]) {
           mvwaddch(win->curses_win, y+1, x+win->border_left, ' ');
           toggle = 1;
           continue; }
         else
           mvwaddch(win->curses_win, y+1, x+win->border_left, text[z]);
         if ('\n' == text[z+1]) {
           z++;
           toggle = 1; }
         else if (0 == text[z+1]) {
            toggle = 1;
            fin = 1; } }
       else
         mvwaddch(win->curses_win, y+1, x+win->border_left, ' '); } } }

void draw_text_from_bottom (struct Win * win) {
// Draw text in win->data from end/bottom to the top.
  char * text = (char *) win->data;
  int width_av = win->width - 1;
  int height_av = win->height - 1;
  char toggle = 0;
  int x, y, offset;
  int z = -1;
  for (y = 0; 0 == toggle; y++)         // Determine number of lines text would have in a window of available
    for (x = 0; x < width_av; x++) {    // width, but infinite height.
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
  if (y < height_av) {             // Depending on what is bigger, determine start point in window or in text.
    start_y = height_av - y;
    for (y = 0; y < start_y; y++)
      for (x = 0; x < width_av; x++)
        mvwaddch(win->curses_win, y+1, x+win->border_left, ' '); }
  else if (y > height_av) {
    offset = y - height_av;
    for (y = 0; y < offset; y++)
      for (x = 0; x < width_av; x++) {
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
  int height_av = win->height - 1;
  int width_av = win->width - 1;
  struct Map map = * (struct Map *) win->data;
  char * cells = map.cells;
  int width_map_av = map.width - map.offset_x;
  int height_map_av = map.height - map.offset_y;
  int x, y, z;
  for (y = 0; y < height_av; y++) {
    z = map.offset_x + (map.offset_y + y) * (map.width);
    for (x = 0; x < width_av; x++) {
      if (y < height_map_av && x < width_map_av) {
        if (z == (map.width * map.player_y) + map.player_x)
          mvwaddch(win->curses_win, y+1, x+win->border_left, '@');
        else
          mvwaddch(win->curses_win, y+1, x+win->border_left, cells[z]);
        z++; }
      else
        mvwaddch(win->curses_win, y+1, x+win->border_left, ' '); } } }

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

int main () {
  WINDOW * screen = initscr();
  noecho();
  curs_set(0);
  struct WinMeta win_meta = init_win_meta(screen);

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

  char key;
  while (1) {
    key = getch();
    if      (key == 'Q')                                    // quit
      break;
    else if (key == '1')                                    // toggle map window
      toggle_window(&win_meta, &win_map);
    else if (key == '2')                                    // toggle info window
      toggle_window(&win_meta, &win_info);
    else if (key == '3')                                    // toggle log window
      toggle_window(&win_meta, &win_log);
    else if (key == 'x') {                                  // scroll map down
      map.offset_y++;
      draw_all_windows (&win_meta); }
    else if (key == 'w' && map.offset_y > 0) {              // scroll map up
      map.offset_y--;
      draw_all_windows (&win_meta); }
    else if (key == 'd') {                                  // scroll map right
      map.offset_x++;
      draw_all_windows (&win_meta); }
    else if (key == 'a' && map.offset_x > 0) {              // scroll map left
      map.offset_x--;
      draw_all_windows (&win_meta); }
    else if (key == 'b' && map.player_y < map.height - 1) { // move player south
      update_info (&win_info);
      update_log (&win_log, "\nYou move south.");
      map.player_y++;
      draw_all_windows (&win_meta); }
    else if (key == 't' && map.player_y > 0) {              // move player north
      update_info (&win_info);
      update_log (&win_log, "\nYou move north.");
      map.player_y--;
      draw_all_windows (&win_meta); }
    else if (key == 'h' && map.player_x < map.width - 1) {  // move player east
      update_info (&win_info);
      update_log (&win_log, "\nYou move east.");
      map.player_x++;
      draw_all_windows (&win_meta); }
    else if (key == 'f' && map.player_x > 0) {              // move player west
      update_info (&win_info);
      update_log (&win_log, "\nYou move west.");
      map.player_x--;
      draw_all_windows (&win_meta); }
    else if (key == '.') {                                  // wait
      update_info (&win_info);
      update_log (&win_log, "\nYou wait.");
      draw_all_windows(&win_meta); }
    else if (key == '>' && win_meta.active != 0)            // cycle forwards in windows chain
      cycle_active_window(&win_meta, 'n');
    else if (key == '<' && win_meta.active != 0)            // cycle backwards in windows chain
      cycle_active_window(&win_meta, 'p');
    else if (key == 'y' && win_meta.active != 0)            // shift window forwards
      shift_window(&win_meta, 'f');
    else if (key == 'Y' && win_meta.active != 0)            // shift window backwards
      shift_window(&win_meta, 'b');
    else if (key == '*' && win_meta.active != 0)            // grow window horizontally
      resize_window(&win_meta, '*');
    else if (key == '_' && win_meta.active != 0)            // shrink window horizontally
      resize_window(&win_meta, '_');
    else if (key == '+' && win_meta.active != 0)            // grow window vertically
      resize_window(&win_meta, '+');
    else if (key == '-' && win_meta.active != 0)            // shrink window vertically
      resize_window(&win_meta, '-'); }

  endwin();
  return 0; }
