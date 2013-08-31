/* keybindings.c */



#include "keybindings.h"
#include <stdlib.h>  /* for malloc(), free(), atoi() */
#include <stdint.h>  /* for uint16_t */
#include <ncurses.h> /* for keycode defines in get_keyname() */
#include <string.h>  /* for strchr(), strlen(), strcmp(), memcpy()*/
#include "windows.h" /* for draw_all_wins() and WinMeta struct */
#include "misc.h"    /* for texfile_sizes() */
#include "main.h"    /* for World struct */
#include "rexit.h"   /* for err_exit() */



extern void init_keybindings(struct World * world)
{
    FILE * file = fopen("config/keybindings", "r");
    uint16_t lines, linemax;
    char * err = "textfile_sizes() in init_keybindings() returns error.";
    exit_err(textfile_sizes(file, &linemax, &lines), world, err);
    struct KeyBinding * keybindings = malloc(lines * sizeof(struct KeyBinding));
    char * command = malloc(linemax);
    uint16_t commcount = 0;
    char * cmdptr;
    while (fgets(command, linemax + 1, file))
    {
        keybindings[commcount].key = atoi(command);
        cmdptr = strchr(command, ' ') + 1;
        keybindings[commcount].name = malloc(strlen(cmdptr));
        memcpy(keybindings[commcount].name, cmdptr, strlen(cmdptr) - 1);
        keybindings[commcount].name[strlen(cmdptr) - 1] = '\0';
        commcount++;
    }
    free(command);
    fclose(file);
    struct KeysWinData * keyswindata = malloc(sizeof(struct KeysWinData));
    keyswindata->max = lines - 1;
    keyswindata->select = 0;
    keyswindata->edit = 0;
    world->keybindings = keybindings;
    world->keyswindata = keyswindata;
}



extern void save_keybindings(struct World * world)
{
    struct KeysWinData * keyswindata = (struct KeysWinData *)
                                       world->keyswindata;
    struct KeyBinding * keybindings = world->keybindings;
    FILE * file = fopen("config/keybindings", "w");
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
    char * line = malloc(linemax);
    for (i = 0; i <= keyswindata->max; i++)
    {
        snprintf(line, linemax,
                 "%d %s\n", keybindings[i].key, keybindings[i].name);
        fwrite(line, sizeof(char), strlen(line), file);
    }
    free(line);
    fclose(file);
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



extern char * get_keyname(uint16_t keycode)
{
    char * keyname;
    keyname = malloc(15);
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
