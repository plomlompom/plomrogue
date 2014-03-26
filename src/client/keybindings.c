/* src/client/keybindings.c */

#include "keybindings.h"
#include <ncurses.h> /* keycode defines, cbreak(), halfdelay(), getch() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include <stdlib.h> /* atoi() */
#include <string.h> /* strlen(), strchr(), strcmp() */
#include "../common/err_try_fgets.h" /* err_try_fgets(), err_line() */
#include "../common/readwrite.h" /* try_fwrite()*/
#include "../common/try_malloc.h" /* try_malloc() */
#include "command_db.h" /* get_command() */
#include "array_append.h" /* array_append() */
#include "windows.h" /* draw_all_wins() */
#include "world.h" /* global world */



/* Return pointer to global keybindings or to keybindings for wingeometry config
 * (c = "g") or winkeys config (c = "k") or active window's keybindings ("w").
 */
static struct KeyBindingDB * char_selected_kb_db(char c);

/* If "keycode_given" equals "keycode_match", copy "keyname_match" to "keyname"
 * and return 1; otherwise return 0.
 */
static uint8_t try_keycode(uint16_t keycode_given, char * keyname,
                           uint16_t keycode_match, char * keyname_match);



static struct KeyBindingDB * char_selected_kb_db(char c)
{
    struct KeyBindingDB * kbdb;
    kbdb = &world.kb_global;
    if      ('g' == c)
    {
        kbdb = &world.kb_wingeom;
    }
    else if ('k' == c)
    {
        kbdb = &world.kb_winkeys;
    }
    else if ('w' == c)
    {
        struct Win * w = get_win_by_id(world.winDB.active);
        kbdb = &w->kb;
    }
    return kbdb;
}



static uint8_t try_keycode(uint16_t keycode_given, char * keyname,
                           uint16_t keycode_match, char * keyname_match)
{
    if (keycode_given == keycode_match)
    {
        sprintf(keyname, "%s", keyname_match);
        return 1;
    }
    return 0;
}



extern struct Command * get_command_to_keycode(struct KeyBindingDB * kbdb,
                                               uint16_t keycode)
{
    uint16_t n_kb;
    for (n_kb = 0; n_kb < kbdb->n_of_kbs; n_kb++)
    {
        if (keycode == kbdb->kbs[n_kb].keycode)
        {
            return kbdb->kbs[n_kb].command;
        }
    }
    return NULL;
}



extern char * get_keyname_to_keycode(uint16_t keycode)
{
    char * f_name = "get_name_to_keycode()";
    char * keyname = try_malloc(10, f_name);        /* max keyname length + 1 */
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



extern void write_keybindings_to_file(FILE * file, struct KeyBindingDB * kbd)
{
    char * f_name = "write_keybindings_to_file()";
    uint16_t linemax = 0;
    uint16_t n_kb;
    for (n_kb = 0; n_kb < kbd->n_of_kbs; n_kb++)
    {
        if (strlen(kbd->kbs[n_kb].command->dsc_short) > linemax)
        {
            linemax = strlen(kbd->kbs[n_kb].command->dsc_short);
        }
    }
    linemax = linemax + 6;            /* + 6 = + 3 digits + ' ' + '\n' + '\0' */
    char line[linemax];
    for (n_kb = 0; n_kb < kbd->n_of_kbs; n_kb++)
    {
        sprintf(line, "%d %s\n",
                kbd->kbs[n_kb].keycode, kbd->kbs[n_kb].command->dsc_short);
        try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    }
    try_fwrite(world.delim, strlen(world.delim), 1, file, f_name);
}



extern void read_keybindings_from_file(char * line, uint32_t linemax,
                                       FILE * file, struct KeyBindingDB * kbdb)
{
    char * context = "Failed reading keybindings from interface config file. ";
    char * err_space    = "Line illegally ends in whitespace.";
    char * err_nospace  = "No whitespace found in line.";
    char * err_int      = "Line starts not with a decimal number in digits.";
    char * err_toolarge = "Keycode number too large, must be below 1000.";
    char * err_cmd      = "No such command in command DB.";
    kbdb->n_of_kbs = 0;
    while (1)
    {
        err_try_fgets(line, linemax, file, context, "0nf");
        if (!strcmp(world.delim, line))
        {
            break;
        }
        err_line(' ' == line[strlen(line) - 2], line, context, err_space);
        char * ptr_space;
        err_line(!(ptr_space = strchr(line, ' ')), line, context, err_nospace);
        uint8_t i = 0;
        err_line(0 == (ptr_space - line), line, context, err_int);
        for (; i < (ptr_space - line); i++)
        {
            err_line(line[i] < '0' || '9' < line[i], line, context, err_int);
        }
        err_line(i > 3, line, context, err_toolarge);

        struct KeyBinding kb;
        line[strlen(line) - 1] = '\0';
        kb.command = get_command(ptr_space + 1);
        err_line(!(kb.command), line, context, err_cmd);
        kb.keycode = atoi(line);
        array_append(kbdb->n_of_kbs, sizeof(struct KeyBinding), (void *) &kb,
                     (void **) kbdb);
        kbdb->n_of_kbs++;
    }
}



extern void mod_selected_keyb(char kb_c)
{
    struct KeyBindingDB * kbdb = char_selected_kb_db(kb_c);
    kbdb->edit = 1;
    draw_all_wins();
    cbreak();
    int keycode = getch();
    halfdelay(world.halfdelay);
    if (keycode < 1000)
    {
        kbdb->kbs[kbdb->select].keycode = keycode;
    }
    kbdb->edit = 0;
}



extern void move_keyb_selection(char kb_c, char dir)
{
    struct KeyBindingDB * kbdb = char_selected_kb_db(kb_c);
    if      ('u' == dir && kbdb->select > 0)
    {
        kbdb->select--;
    }
    else if ('d' == dir && kbdb->select < kbdb->n_of_kbs - 1)
    {
        kbdb->select++;
    }
}
