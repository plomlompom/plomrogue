#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <stdint.h>

struct World;
struct WinMeta;

struct KeyBinding {
  char * name;
  uint16_t key; };

struct KeysWinData {
  uint16_t max;
  char edit;
  uint16_t select; };

extern void init_keybindings(struct World *);
extern void save_keybindings(struct World *);
extern uint16_t get_action_key (struct KeyBinding *, char *);
extern char * get_keyname(uint16_t);
extern void keyswin_mod_key (struct World *, struct WinMeta *);
extern void keyswin_move_selection (struct World *, char);

#endif
