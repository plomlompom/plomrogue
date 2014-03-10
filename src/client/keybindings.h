/* src/client/keybindings.h
 *
 * Database of keybindings and functions to read and manipulate it.
 */

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <stdint.h> /* uint8_t, uint16_t */
#include <stdio.h> /* FILE */
struct Command;



struct KeyBinding
{
  uint16_t keycode;
  struct Command * command; /* command in command DB to which key is bound */
};

struct KeyBindingDB
{
    struct KeyBinding * kbs;
    uint16_t n_of_kbs; /* how many KeyBinding structs are stored below .kbs? */
    uint16_t select; /* linear list index of keybinding selected for editing */
    uint8_t edit;    /* 1 if currently editing a keybinding, else 0 */
};



/* Return command bound to "keycode" in "kbdb"; NULL if none found. */
extern struct Command * get_command_to_keycode(struct KeyBindingDB * kbdb,
                                               uint16_t keycode);

/* Return human-readable name (of maximum 9 chars) for "keycode" as matched by
 * ncurses.h; if none is found, return "UNKNOWN".
 */
extern char * get_keyname_to_keycode(uint16_t keycode);

/* Read/write from/to "file" "kbd", delimited by world.delim. */
extern void write_keybindings_to_file(FILE * file, struct KeyBindingDB * kbd);
extern void read_keybindings_from_file(char * line, uint32_t linemax,
                                       FILE * file, struct KeyBindingDB * kbd);

/* Mark keybinding in KeybindingDB (char_selected_kb_db()-) selected by "kb_c"
 * as being edited, get user input to modify it, then unmark it again. Ensure
 * there are max. three digits in the ASCII string of the kecode read from user.
 */
extern void mod_selected_keyb(char kb_c);

/* Move .select in KeybindingDB (char-selected_kb_db()-) selected by "kb_c"
 * upwards ("dir"=="u") or downwards ("dir"=="d") within KeyBindingDB limits.
 */
extern void move_keyb_selection(char kb_c, char dir);



#endif
