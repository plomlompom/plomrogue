#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>
#include <string.h>
#include "windows.h"
#include "draw_wins.h"
#include "roguelike.h"
#include "keybindings.h"

void draw_with_linebreaks (struct Win * win, char * text, uint16_t start_y) {
// Write text into window content space. Start on row start_y. Fill unused rows with whitespace.
  uint16_t x, y;
  char toggle;
  char fin = 0;
  int16_t z = -1;
  for (y = start_y; y < win->frame.size.y; y++) {
    if (0 == fin)
      toggle = 0;
    for (x = 0; x < win->frame.size.x; x++) {
       if (0 == toggle) {
         z++;
         if ('\n' == text[z]) {
           toggle = 1;
           continue; }
         else
           mvwaddch(win->frame.curses_win, y, x, text[z]);
         if ('\n' == text[z+1]) {
           z++;
           toggle = 1; }
         else if (0 == text[z+1]) {
            toggle = 1;
            fin = 1; } } } } }

void draw_text_from_bottom (struct Win * win, char * text) {
// Draw text from end/bottom to the top.
  char toggle = 0;
  uint16_t x, y, offset;
  int16_t z = -1;
  for (y = 0; 0 == toggle; y++)                           // Determine number of lines text would have in
    for (x = 0; x < win->frame.size.x; x++) {             // a window of available width, but infinite height.
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
  uint16_t start_y = 0;
  if (y < win->frame.size.y)       // Depending on what is bigger, determine start point in window or in text.
    start_y = win->frame.size.y - y;
  else if (y > win->frame.size.y) {
    offset = y - win->frame.size.y;
    for (y = 0; y < offset; y++)
      for (x = 0; x < win->frame.size.x; x++) {
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
  struct Monster * monster;
  char * cells = map->cells;
  uint16_t width_map_av  = map->width  - map->offset_x;
  uint16_t height_map_av = map->height - map->offset_y;
  uint16_t x, y, z;
  for (y = 0; y < win->frame.size.y; y++) {
    z = map->offset_x + (map->offset_y + y) * (map->width);
    for (x = 0; x < win->frame.size.x; x++) {
      if (y < height_map_av && x < width_map_av) {
          mvwaddch(win->frame.curses_win, y, x, cells[z]);
        z++; } } }
  if (   player->pos.y >= map->offset_y && player->pos.y < map->offset_y + win->frame.size.y
      && player->pos.x >= map->offset_x && player->pos.x < map->offset_x + win->frame.size.x)
    mvwaddch(win->frame.curses_win, player->pos.y - map->offset_y, player->pos.x - map->offset_x, '@');
  for (monster = world->monster; monster != 0; monster = monster->next)
    if (   monster->pos.y >= map->offset_y && monster->pos.y < map->offset_y + win->frame.size.y
        && monster->pos.x >= map->offset_x && monster->pos.x < map->offset_x + win->frame.size.x)
      mvwaddch(win->frame.curses_win, monster->pos.y - map->offset_y, monster->pos.x - map->offset_x, monster->name); }

void draw_info_win (struct Win * win) {
// Draw info window by appending win->data integer value to "Turn: " display.
  struct World * world = (struct World *) win->data;
  uint16_t count = world->turn;
  char text[100];
  snprintf(text, 100, "Turn: %d", count);
  draw_with_linebreaks(win, text, 0); }

void draw_keys_win (struct Win * win) {
// Draw keybindings window.
  struct World * world = (struct World *) win->data;
  uint16_t offset = 0, y, x;
  if (world->keyswindata->max >= win->frame.size.y) {
    if (world->keyswindata->select > win->frame.size.y / 2) {
      if (world->keyswindata->select < (world->keyswindata->max - (win->frame.size.y / 2)))
        offset = world->keyswindata->select - (win->frame.size.y / 2);
      else
        offset = world->keyswindata->max - win->frame.size.y + 1; } }
  uint8_t keydescwidth = 9 + 1; // max length assured by get_keyname() + \0
  char * keydesc = malloc(keydescwidth), * keyname;
  attr_t attri;
  for (y = 0; y <= world->keyswindata->max && y < win->frame.size.y; y++) {
    if (0 == y && offset > 0) {
      draw_scroll_hint(&win->frame, y, offset + 1, '^');
      continue; }
    else if (win->frame.size.y == y + 1 && 0 < world->keyswindata->max - (win->frame.size.y + offset - 1)) {
      draw_scroll_hint(&win->frame, y, world->keyswindata->max - (offset + win->frame.size.y) + 2, 'v');
      continue; }
    attri = 0;
    if (y == world->keyswindata->select - offset) {
      attri = A_REVERSE;
      if (1 == world->keyswindata->edit)
        attri = attri | A_BLINK; }
    keyname = get_keyname(world->keybindings[y + offset].key);
    snprintf(keydesc, keydescwidth, "%-9s", keyname);
    free(keyname);
    for (x = 0; x < win->frame.size.x; x++)
      if (x < strlen(keydesc))
        mvwaddch(win->frame.curses_win, y, x, keydesc[x] | attri);
      else if (strlen(keydesc) < x && x < strlen(world->keybindings[y + offset].name) + strlen(keydesc) + 1)
        mvwaddch(win->frame.curses_win, y, x,
                 world->keybindings[y + offset].name[x - strlen(keydesc) - 1] | attri);
      else
        mvwaddch(win->frame.curses_win, y, x, ' ' | attri); }
  free(keydesc); }