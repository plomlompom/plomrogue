/* keybindings.h
 *
 * Retrieval and storage of keybindings.
 */

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <stdint.h> /* for uint8_t, uint16_t */



/* Individual keybinding in keybinding chain. */
struct KeyBinding
{
  struct KeyBinding * next;
  uint16_t key; /* keycode */
  char * name;  /* name of functionality bound to keycode */
};

/* Wrapper to keybinding chain, contains some keybinding editing metadata. */
struct KeyBiData
{
    struct KeyBinding * kbs;
    uint8_t edit;    /* 1 if currently editing a keybinding, else 0 */
    uint16_t select; /* linear list index of keybinding selected for editing */
};



/* Return name of action / functionality coupled to keycode; NULL on failure. */
extern char * get_func_to_keycode(struct KeyBinding * kb_p, uint16_t key);

/* Return keycode matched by keybinding to command of "name". */
extern uint16_t get_keycode_to_action(struct KeyBinding * keybindings,
                                      char * name);

/* Return human-readable name (of maximum 9 chars) for "keycode" as matched by
 * ncurses.h; if none is found, return "UNKNOWN".
 */
extern char * get_name_to_keycode(uint16_t keycode);

/* Return number of keybindings in keybindings chain from "kb_p" on. */
extern uint16_t get_n_of_keybs(struct KeyBinding * kb_p);

/* Return "n"-th keybinding in keybindings chain from "kb_p" on. */
extern struct KeyBinding * get_keyb_of_n(struct KeyBinding * kb_p, uint16_t n);

/* Initialize/save keybindings data from/to file at "path" to/from keybindings
 * data pointer "kbd".
 */
extern void init_keybindings(char * path, struct KeyBiData * kbd);
extern void save_keybindings(char * path, struct KeyBiData * kbd);

/* Free keybinding chain starting at "kb_start". */
extern void free_keybindings(struct KeyBinding * kb_start);

/* Mark keybinding selected for modification as being edited, get user input to
 * modify it, then unmark it again. Ensure there are max. three digits in the
 * keycode ASCII representation.
 */
extern void mod_selected_keyb(struct KeyBiData * kbd);

/* Move keybinding modification selection upwards ("dir"=="u") or downwards
 * ("dir"=="d") within the limits of the keybindings chain length.
 */
extern void move_keyb_mod_selection(struct KeyBiData * kbd, char dir);



#endif
