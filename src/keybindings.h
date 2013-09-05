/* keybindings.h
 *
 * Retrieval and storage of keybindings.
 */

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H



#include <stdint.h> /* for uint16_t */
struct World;
struct WinMeta;



/* Individual keybinding. */
struct KeyBinding
{
  char * name;  /* name of functionality bound to keycode */
  uint16_t key; /* keycode */
};



/* Metadata used by the keybinding editing window. */
struct KeysWinData
{
  uint16_t max;    /* index of last keybinding (= n of keybindings - 1) */
  char edit;       /* 1 if currently editing a keybinding, else 0 */
  uint16_t select; /* index of keybinding selected for editing */
};



/* Read keybindings data from / write them to the file "keybindings". */
extern void init_keybindings(struct World * world);
extern void save_keybindings(struct World * world);



/* Return keycode matching a key (functionality) name. */
extern uint16_t get_action_key(struct KeyBinding * keybindings, char * name);



/* Translate keycode to readable names of max 9 chars where possible. */
extern char * get_keyname(struct World * world, uint16_t keycode);



/* Mark selection in keybindings window modifiable, get user input to modify
 * key. Ensure there are max. three digits in the keycode ASCII representation.
 */
extern void keyswin_mod_key(struct World * world, struct WinMeta * win_meta);



/* Move selection in keybinding window upwards ("dir" = "u") or downwards ("dir"
 * = "d") within the limits of the keybindings list length.
 */
extern void keyswin_move_selection(struct World * world, char dir);



#endif
