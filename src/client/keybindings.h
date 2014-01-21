/* src/client/keybindings.h
 *
 * Database of keybindings and functions to read and manipulate it.
 */

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <stdint.h> /* uint8_t, uint16_t */



struct KeyBinding
{
  struct KeyBinding * next;
  uint16_t key;   /* keycode */
  char * command; /* name of command / functionality bound to keycode */
};

struct KeyBindingDB
{
    struct KeyBinding * kbs;
    uint16_t select; /* linear list index of keybinding selected for editing */
    uint8_t edit;    /* 1 if currently editing a keybinding, else 0 */
};



/* Return name of command / functionality bound to keycode; NULL on failure. */
extern char * get_command_to_keycode(struct KeyBinding * kb_p, uint16_t key);

/* Return keycode bound to "command". */
extern uint16_t get_keycode_to_command(struct KeyBinding * keybindings,
                                       char * command);

/* Return human-readable name (of maximum 9 chars) for "keycode" as matched by
 * ncurses.h; if none is found, return "UNKNOWN".
 */
extern char * get_keyname_to_keycode(uint16_t keycode);

/* Initialize/save keybindings data from/to file at "path" to/from keybindings
 * data pointer "kbd".
 */
extern void init_keybindings(char * path, struct KeyBindingDB * kbd);
extern void save_keybindings(char * path, struct KeyBindingDB * kbd);

/* Free keybinding chain starting at "kb_start". */
extern void free_keybindings(struct KeyBinding * kb_start);

/* Mark keybinding selected for modification as being edited, get user input to
 * modify it, then unmark it again. Ensure there are max. three digits in the
 * ASCII representation of the keycode read from the user.
 */
extern void mod_selected_keyb(struct KeyBindingDB * kbd);

/* Move keybinding modification selection upwards ("dir"=="u") or downwards
 * ("dir"=="d") within the limits of the keybindings chain length.
 */
extern void move_keyb_mod_selection(struct KeyBindingDB * kbd, char dir);



#endif
