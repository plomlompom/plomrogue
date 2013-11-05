/* keybindings.c */

#include "keybindings.h"
#include <stdio.h>     /* for FILE typedef*/
#include <stdlib.h>    /* for free(), atoi() */
#include <stdint.h>    /* for uint16_t */
#include <ncurses.h>   /* for keycode defines in get_name_to_keycode() */
#include <string.h>    /* for strchr(), strlen(), strcmp(), memcpy()*/
#include "windows.h"   /* for draw_all_wins() and WinMeta struct */
#include "readwrite.h" /* for texfile_sizes(), try_fopen(), try_fclose()
                        * try_fclose_unlink_rename(), try_fwrite()
                        */
#include "main.h"      /* for world global */
#include "rexit.h"     /* for err_exit() */
#include "misc.h"      /* for try_malloc() */



/* If "keycode_given" equals "keycode_match", copy "keyname_match" to "keyname"
 * and return 1; otherwise return 0.
 */
static uint8_t try_keycode(uint16_t keycode_given, char * keyname,
                           uint16_t keycode_match, char * keyname_match);



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



extern uint16_t get_keycode_to_action(struct KeyBinding * kb_p, char * name)
{
    while (0 != kb_p)
    {
        if (0 == strcmp(kb_p->name, name))
        {
            return kb_p->key;
        }
        kb_p = kb_p->next;
    }
    return 0;
}



extern char * get_name_to_keycode(uint16_t keycode)
{
    char * f_name = "get_name_to_keycode()";
    char * keyname = try_malloc(15, f_name);
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



extern uint16_t get_n_of_keybs(struct KeyBinding * kb_p)
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



extern struct KeyBinding * get_keyb_of_n(struct KeyBinding * kb_p, uint16_t n)
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



extern void init_keybindings(char * path, struct KeyBiData * kbd)
{
    char * f_name = "init_keybindings()";

    FILE * file = try_fopen(path, "r", f_name);
    uint16_t lines, linemax;
    char * err = "textfile_sizes() in init_keybindings() returns error.";
    exit_err(textfile_sizes(file, &linemax, &lines), err);

    char command[linemax + 1];
    char * cmdptr;
    struct KeyBinding ** loc_last_ptr = &kbd->kbs;
    * loc_last_ptr = 0;
    while (fgets(command, linemax + 1, file))
    {
        if ('\n' == command[0] || 0 == command[0])
        {
            break;
        }
        * loc_last_ptr = try_malloc(sizeof(struct KeyBinding), f_name);
        struct KeyBinding * kb_p = * loc_last_ptr;
        kb_p->next = 0;
        kb_p->key = atoi(command);
        cmdptr = strchr(command, ' ') + 1;
        kb_p->name = try_malloc(strlen(cmdptr), f_name);
        memcpy(kb_p->name, cmdptr, strlen(cmdptr) - 1);
        kb_p->name[strlen(cmdptr) - 1] = '\0';
        loc_last_ptr = & kb_p->next;
    }

    try_fclose(file, f_name);

    kbd->edit = 0;
    kbd->select = 0;
}



extern void save_keybindings(char * path, struct KeyBiData * kbd)
{
    char * f_name = "save_keybindings()";

    char path_tmp[strlen(path) + 4 + 1];
    sprintf(path_tmp, "%s_tmp", path);
    FILE * file = try_fopen(path_tmp, "w", f_name);

    uint16_t linemax = 0;
    struct KeyBinding * kb_p = kbd->kbs;
    while (0 != kb_p)
    {
        if (strlen(kb_p->name) > linemax)
        {
            linemax = strlen(kb_p->name);
        }
        kb_p = kb_p->next;
    }
    linemax = linemax + 6;         /* + 6 = + 3 digits + whitespace + \n + \0 */

    char line[linemax];
    kb_p = kbd->kbs;
    while (0 != kb_p)
    {
        snprintf(line, linemax, "%d %s\n", kb_p->key, kb_p->name);
        try_fwrite(line, sizeof(char), strlen(line), file, f_name);
        kb_p = kb_p->next;
    }

    try_fclose_unlink_rename(file, path_tmp, path, f_name);
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
    free(kb_start->name);
    free(kb_start);
}



extern void mod_selected_keyb(struct KeyBiData * kbd)
{
    kbd->edit = 1;
    char * err = "Trouble with draw_all_wins() in mod_selected_keyb().";
    exit_err(draw_all_wins(world.wmeta), err);
    int key = getch();
    if (key < 1000)
    {
        struct KeyBinding * kb_p = get_keyb_of_n(kbd->kbs, kbd->select);
        kb_p->key = key;
    }
    kbd->edit = 0;
}



extern void move_keyb_mod_selection(struct KeyBiData * kbd, char dir)
{
    if      ('u' == dir && kbd->select > 0)
    {
        kbd->select--;
    }
    else if ('d' == dir && kbd->select < get_n_of_keybs(kbd->kbs) - 1)
    {
        kbd->select++;
    }
}
