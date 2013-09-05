/* keybindings.c */

#include "keybindings.h"
#include <stdio.h>     /* for FILE typedef*/
#include <stdlib.h>    /* for free(), atoi() */
#include <stdint.h>    /* for uint16_t */
#include <ncurses.h>   /* for keycode defines in get_keyname() */
#include <string.h>    /* for strchr(), strlen(), strcmp(), memcpy()*/
#include "windows.h"   /* for draw_all_wins() and WinMeta struct */
#include "readwrite.h" /* for texfile_sizes(), try_fopen(), try_fclose()
                        * try_fclose_unlink_rename()
                        */
#include "main.h"      /* for World struct */
#include "rexit.h"     /* for err_exit() */
#include "misc.h"      /* for try_malloc() */



extern void init_keybindings(struct World * world)
{
    char * f_name = "init_keybindings()";
    char * path = "config/keybindings";
    FILE * file = try_fopen(path, "r", world, f_name);
    uint16_t lines, linemax;
    char * err = "textfile_sizes() in init_keybindings() returns error.";
    exit_err(textfile_sizes(file, &linemax, &lines), world, err);
    struct KeyBinding * keybindings;
    keybindings = try_malloc(lines * sizeof(struct KeyBinding), world, f_name);
    char command[linemax + 1];
    uint16_t commcount = 0;
    char * cmdptr;
    while (fgets(command, linemax + 1, file))
    {
        keybindings[commcount].key = atoi(command);
        cmdptr = strchr(command, ' ') + 1;
        keybindings[commcount].name = try_malloc(strlen(cmdptr), world, f_name);
        memcpy(keybindings[commcount].name, cmdptr, strlen(cmdptr) - 1);
        keybindings[commcount].name[strlen(cmdptr) - 1] = '\0';
        commcount++;
    }
    try_fclose(file, world, f_name);
    struct KeysWinData * keyswindata;
    keyswindata = try_malloc(sizeof(struct KeysWinData), world, f_name);
    keyswindata->max = lines - 1;
    keyswindata->select = 0;
    keyswindata->edit = 0;
    world->keybindings = keybindings;
    world->keyswindata = keyswindata;
}



extern void save_keybindings(struct World * world)
{
    char * f_name = "save_keybindings()";
    struct KeysWinData * keyswindata = (struct KeysWinData *)
                                       world->keyswindata;
    struct KeyBinding * keybindings = world->keybindings;
    char * path     = "config/keybindings";
    char * path_tmp = "config/keybindings_tmp";
    FILE * file = try_fopen(path_tmp, "w", world, f_name);
    uint16_t linemax = 0;
    uint16_t i;
    for (i = 0; i <= keyswindata->max; i++)
    {
        if (strlen(keybindings[i].name) > linemax)
        {
            linemax = strlen(keybindings[i].name);
        }
    }
    linemax = linemax + 6;         /* + 6 = + 3 digits + whitespace + \n + \0 */
    char line[linemax];
    for (i = 0; i <= keyswindata->max; i++)
    {
        snprintf(line, linemax,
                 "%d %s\n", keybindings[i].key, keybindings[i].name);
        fwrite(line, sizeof(char), strlen(line), file);
    }
    try_fclose_unlink_rename(file, path_tmp, path, world, f_name);
}



extern uint16_t get_action_key(struct KeyBinding * keybindings, char * name)
{
    uint16_t i = 0;
    while (strcmp(keybindings[i].name, name) )
    {
        i++;
    }
    return keybindings[i].key;
}



extern char * get_keyname(struct World * world, uint16_t keycode)
{
    char * f_name = "get_keyname()";
    char * keyname = try_malloc(15, world, f_name);
    if (32 < keycode && keycode < 127)
    {
        sprintf(keyname, "%c", keycode);
    }
    else if (keycode == 9)              /* TODO: Find a way more elegant than */
    {                                   /*       hardcoding all of this?      */
        sprintf(keyname, "TAB");
    }
    else if (keycode == 10)
    {
        sprintf(keyname, "RETURN");
    }
    else if (keycode == 27)
    {
        sprintf(keyname, "ESCAPE");
    }
    else if (keycode == 32)
    {
        sprintf(keyname, "SPACE");
    }
    else if (keycode == KEY_UP)
    {
        sprintf(keyname, "UP");
    }
    else if (keycode == KEY_DOWN)
    {
        sprintf(keyname, "DOWN");
    }
    else if (keycode == KEY_LEFT)
    {
        sprintf(keyname, "LEFT");
    }
    else if (keycode == KEY_RIGHT)
    {
        sprintf(keyname, "RIGHT");
    }
    else if (keycode == KEY_HOME)
    {
        sprintf(keyname, "HOME");
    }
    else if (keycode == KEY_BACKSPACE)
    {
        sprintf(keyname, "BACKSPACE");
    }
    else if (keycode >= KEY_F0 && keycode <= KEY_F(63))
    {
        uint16_t f = keycode - KEY_F0;
        sprintf(keyname, "F%d", f);
    }
    else if (keycode == KEY_DC)
    {
        sprintf(keyname, "DELETE");
    }
    else if (keycode == KEY_IC)
    {
        sprintf(keyname, "INSERT");
    }
    else if (keycode == KEY_NPAGE)
    {
        sprintf(keyname, "NEXT PAGE");
    }
    else if (keycode == KEY_PPAGE)
    {
        sprintf(keyname, "PREV PAGE");
    }
    else if (keycode == KEY_END)
    {
        sprintf(keyname, "END");
    }
    else
    {
        sprintf(keyname, "(unknown)");
    }
    return keyname;
}



extern void keyswin_mod_key(struct World * world, struct WinMeta * win_meta)
{
    world->keyswindata->edit = 1;
    exit_err(draw_all_wins(win_meta), world, "Trouble with draw_all_wins() in "
                                             "keyswin_mod_key().");
    int key = getch();
    if (key < 1000)
    {
        world->keybindings[world->keyswindata->select].key = key;
    }
    world->keyswindata->edit = 0;
}



extern void keyswin_move_selection(struct World * world, char dir)
{
    if      ('u' == dir && world->keyswindata->select > 0)
    {
        world->keyswindata->select--;
    }
    else if ('d' == dir && world->keyswindata->select < world->keyswindata->max)
    {
        world->keyswindata->select++;
    }
}
