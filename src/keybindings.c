#include "keybindings.h"
#include <stdint.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include "windows.h"
#include "roguelike.h"

void init_keybindings(struct World * world) {
// Initialize keybindings from file "keybindings".
  FILE * file = fopen("keybindings", "r");
  uint16_t lines = 0;
  int c = 0;
  uint16_t linemax = 0;
  uint16_t c_count = 0;
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
  uint16_t commcount = 0;
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

void save_keybindings(struct World * world) {
// Write keybindings to keybindings file.
  struct KeysWinData * keyswindata = (struct KeysWinData *) world->keyswindata;
  struct KeyBinding * keybindings = world->keybindings;
  FILE * file = fopen("keybindings", "w");
  uint16_t linemax = 0;
  uint16_t i;
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

uint16_t get_action_key (struct KeyBinding * keybindings, char * name) {
// Return key matching name in keybindings.
  uint16_t i = 0;
  while (strcmp(keybindings[i].name, name) )
    i++;
  return keybindings[i].key; }

char * get_keyname(uint16_t keycode) {
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
    uint16_t f = keycode - KEY_F0;
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

void keyswin_mod_key (struct World * world, struct WinMeta * win_meta) {
// In keybindings window, mark selection modifiable, modify key. Ensure max of three digits in key code field.
  world->keyswindata->edit = 1;
  draw_all_wins (win_meta);
  int key = getch();
  if (key < 1000)
    world->keybindings[world->keyswindata->select].key = key;
  world->keyswindata->edit = 0; }

void keyswin_move_selection (struct World * world, char dir) {
// In keybindings window, move selection upwards or downwards (if within limits of list length).
  if ('u' == dir && world->keyswindata->select > 0)
    world->keyswindata->select--;
  else if ('d' == dir && world->keyswindata->select < world->keyswindata->max)
    world->keyswindata->select++; }
