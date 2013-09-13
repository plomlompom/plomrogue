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
  struct KeyBinding * next;
  uint16_t key; /* keycode */
  char * name;  /* name of functionality bound to keycode */
};



/* Metadata used by the keybinding editing window. */
struct KeysWinData
{
  char edit;       /* 1 if currently editing a keybinding, else 0 */
  uint16_t select; /* list index of keybinding selected for editing */
};



/* Return keycode matched by keybinding to command of "name". */
extern uint16_t get_keycode_to_action(struct KeyBinding * keybindings, char * name);

/* Return human-readable name (of maximum 9 chars) for "keycode" as matched by
 * ncurses.h; if none is found, return "UNKNOWN". */
extern char * get_name_to_keycode(struct World * world, uint16_t keycode);

/* Return number of keybindings in keybindings chain. */
extern uint16_t get_n_of_keybs(struct World * world);

/* Return "n"-th keybinding in keybindings chain. */
extern struct KeyBinding * get_keyb_of_n(struct World * world, uint16_t n);



/* Initialize keybindings data (by reading from file "keybindings"), save it (by
 * writing to the same file) and free it.
 */
extern void init_keybindings(struct World * world);
extern void save_keybindings(struct World * world);
extern void free_keybindings(struct KeyBinding * kb_start);



/* Mark selection in keybindings window modifiable, get user input to modify
 * key. Ensure there are max. three digits in the keycode ASCII representation.
 */
extern void keyswin_mod_key(struct World * world, struct WinMeta * win_meta);

/* Move selection in keybinding window upwards ("dir" = "u") or downwards ("dir"
 * = "d") within the limits of the keybindings list length.
 */
extern void move_keys_mod_selection(struct World * world, char dir);



#endif
