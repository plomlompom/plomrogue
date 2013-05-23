#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include "windows.h"
#include "draw_wins.h"
#include "roguelike.h"
#include "keybindings.h"

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

void draw_log_win (struct Win * win) {
// Draw log text from world struct in win->data from bottom to top.
  struct World * world = (struct World *) win->data;
  draw_text_from_bottom(win, world->log); }

void draw_map_win (struct Win * win) {
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

void draw_info_win (struct Win * win) {
// Draw info window by appending win->data integer value to "Turn: " display.
  struct World * world = (struct World *) win->data;
  int count = world->turn;
  char text[100];
  snprintf(text, 100, "Turn: %d", count);
  draw_with_linebreaks(win, text, 0); }

void draw_keys_win (struct Win * win) {
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