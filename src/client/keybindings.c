/* src/client/keybindings.c */

#include "keybindings.h"
#include <ncurses.h> /* keycode defines, cbreak(), halfdelay(), getch() */
#include <stddef.h> /* NULL */
#include <stdio.h> /* FILE, sprintf(), snprintf() */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* strlen(), strchr(), strcmp() */
#include "../common/readwrite.h" /* textfile_sizes(), try_fgets(),try_fwrite()*/
#include "../common/try_malloc.h" /* try_malloc() */
#include "windows.h" /* draw_all_wins() */
#include "world.h" /* global world */



/* Return "n"-th keybinding in keybindings chain from "kb_p" on. */
static struct KeyBinding * get_keyb_of_n(struct KeyBinding * kb_p, uint16_t n);

/* Return number of keybindings in keybindings chain from "kb_p" on. */
static uint16_t get_n_of_keybs(struct KeyBinding * kb_p);

/* Return pointer to global keybindings or to keybindings for wingeometry config
 * (c = "g") or winkeys config (c = "k") or active window's keybindings ("w").
 */
static struct KeyBindingDB * char_selected_kb_db(char c);

/* If "keycode_given" equals "keycode_match", copy "keyname_match" to "keyname"
 * and return 1; otherwise return 0.
 */
static uint8_t try_keycode(uint16_t keycode_given, char * keyname,
                           uint16_t keycode_match, char * keyname_match);



static struct KeyBinding * get_keyb_of_n(struct KeyBinding * kb_p, uint16_t n)
{
    uint16_t i = 0;
    while (1)
    {
        if (n == i)
        {
            break;
        }
        i++;
        kb_p = kb_p->next;
    }
    return kb_p;
}



static uint16_t get_n_of_keybs(struct KeyBinding * kb_p)
{
    uint16_t i = 0;
    while (1)
    {
        if (0 == kb_p)
        {
            break;
        }
        i++;
        kb_p = kb_p->next;
    }
    return i;
}



static struct KeyBindingDB * char_selected_kb_db(char c)
{
    struct KeyBindingDB * kbd;
    kbd = &world.kb_global;
    if      ('g' == c)
    {
        kbd = &world.kb_wingeom;
    }
    else if ('k' == c)
    {
        kbd = &world.kb_winkeys;
    }
    else if ('w' == c)
    {
        struct Win * w = get_win_by_id(world.winDB.active);
        kbd = &w->kb;
    }
    return kbd;
}



static uint8_t try_keycode(uint16_t keycode_given, char * keyname,
                           uint16_t keycode_match, char * keyname_match)
{
    if (keycode_given == keycode_match)
    {
        sprintf(keyname, keyname_match);
        return 1;
    }
    return 0;
}



extern struct Command * get_command_to_keycode(struct KeyBinding * kb_p,
                                               uint16_t key)
{
    while (0 != kb_p)
    {
        if (key == kb_p->key)
        {
            return kb_p->command;
        }
        kb_p = kb_p->next;
    }
    return NULL;
}



extern char * get_keyname_to_keycode(uint16_t keycode)
{
    char * f_name = "get_name_to_keycode()";
    char * keyname = try_malloc(15, f_name);                /* FIXME: Why 15? */
    if (32 < keycode && keycode < 127)
    {
        sprintf(keyname, "%c", keycode);
    }
    else if (keycode >= KEY_F0 && keycode <= KEY_F(63))
    {
        uint16_t f = keycode - KEY_F0;
        sprintf(keyname, "F%d", f);
    }
    else if (   try_keycode(keycode, keyname, 9, "TAB")
             || try_keycode(keycode, keyname, 10, "RETURN")
             || try_keycode(keycode, keyname, 27, "ESCAPE")
             || try_keycode(keycode, keyname, 32, "SPACE")
             || try_keycode(keycode, keyname, KEY_UP, "UP")
             || try_keycode(keycode, keyname, KEY_DOWN, "DOWN")
             || try_keycode(keycode, keyname, KEY_LEFT, "LEFT")
             || try_keycode(keycode, keyname, KEY_RIGHT, "RIGHT")
             || try_keycode(keycode, keyname, KEY_HOME, "HOME")
             || try_keycode(keycode, keyname, KEY_BACKSPACE, "BACKSPACE")
             || try_keycode(keycode, keyname, KEY_DC, "DELETE")
             || try_keycode(keycode, keyname, KEY_IC, "INSERT")
             || try_keycode(keycode, keyname, KEY_NPAGE, "NEXT PAGE")
             || try_keycode(keycode, keyname, KEY_PPAGE, "PREV PAGE")
             || try_keycode(keycode, keyname, KEY_END, "END"))
    {
        ;
    }
    else
    {
        sprintf(keyname, "(unknown)");
    }
    return keyname;
}



extern void write_keybindings_to_file(FILE * file, struct KeyBindingDB * kbd,
                                      char * delim)
{
    char * f_name = "write_keybindings_to_file()";
    uint16_t linemax = 0;
    struct KeyBinding * kb_p = kbd->kbs;
    while (0 != kb_p)
    {
        if (strlen(kb_p->command->dsc_short) > linemax)
        {
            linemax = strlen(kb_p->command->dsc_short);
        }
        kb_p = kb_p->next;
    }
    linemax = linemax + 6;            /* + 6 = + 3 digits + ' ' + '\n' + '\0' */
    char line[linemax];
    kb_p = kbd->kbs;
    while (0 != kb_p)
    {
        sprintf(line, "%d %s\n", kb_p->key, kb_p->command->dsc_short);
        try_fwrite(line, sizeof(char), strlen(line), file, f_name);
        kb_p = kb_p->next;
    }
    try_fwrite(delim, strlen(delim), 1, file, f_name);
}



extern void read_keybindings_from_file(char * line, uint32_t linemax,
                                       FILE * file, struct KeyBindingDB * kbd)
{
    char * f_name = "read_keybindings_from_file()";
    char * cmdptr;
    struct KeyBinding ** loc_last_ptr = &kbd->kbs;
    * loc_last_ptr = 0;
    while (try_fgets(line, linemax + 1, file, f_name))
    {
        if (!strcmp("%\n", line))
        {
            break;
        }
        * loc_last_ptr = try_malloc(sizeof(struct KeyBinding), f_name);
        struct KeyBinding * kb_p = * loc_last_ptr;
        kb_p->next = 0;
        kb_p->key = atoi(line);
        cmdptr = strchr(line, ' ') + 1;
        cmdptr[strlen(cmdptr) - 1] = '\0';
        kb_p->command = get_command(cmdptr);
        loc_last_ptr = & kb_p->next;
    }
}



extern void free_keybindings(struct KeyBinding * kb_start)
{
    if (0 == kb_start)
    {
        return;
    }
    struct KeyBinding * kb_p = kb_start->next;
    if (0 != kb_p)
    {
        free_keybindings(kb_p);
    }
    free(kb_start);
}



extern void mod_selected_keyb(char kb_c)
{
    struct KeyBindingDB * kbd = char_selected_kb_db(kb_c);
    kbd->edit = 1;
    draw_all_wins();
    cbreak();
    int key = getch();
    halfdelay(world.halfdelay);
    if (key < 1000)
    {
        struct KeyBinding * kb_p = get_keyb_of_n(kbd->kbs, kbd->select);
        kb_p->key = key;
    }
    kbd->edit = 0;
}



extern void move_keyb_selection(char kb_c, char dir)
{
    struct KeyBindingDB * kbd = char_selected_kb_db(kb_c);
    if      ('u' == dir && kbd->select > 0)
    {
        kbd->select--;
    }
    else if ('d' == dir && kbd->select < get_n_of_keybs(kbd->kbs) - 1)
    {
        kbd->select++;
    }
}
