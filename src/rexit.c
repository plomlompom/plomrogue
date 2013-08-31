/* rexit.c */

#include "rexit.h"
#include <stdlib.h> /* for exit(), free(), defines EXIT_SUCESS, EXIT_FAILURE */
#include <stdio.h> /* for printf(), perror() */
#include <stdint.h> /* for uint8_t */
#include <ncurses.h> /* for endwin() */
#include <errno.h> /* for errno */
#include "main.h" /* for World struct */
#include "map.h" /* for Map struct */
#include "keybindings.h" /* for KeysWinData, KeyBinding structs */
#include "command_db.h" /* for free_command_db() */
#include "windows.h" /* for Win struct, free_win() */
#include "map_objects.h" /* for free_item_defs(), free_monster_defs() */



/* The clean-up routine and the flag resource by which it decides what to do. */
static uint32_t cleanup_flags = 0x0000;
static void cleanup(struct World * world);



static void cleanup(struct World * world)
{
    if (cleanup_flags & CLEANUP_NCURSES)
    {
        endwin();
    }
    if (cleanup_flags & CLEANUP_MAP)
    {
        free(world->map->cells);
    }
    if (cleanup_flags & CLEANUP_KEYBINDINGS)
    {
        uint16_t key;
        for (key = 0; key <= world->keyswindata->max; key++)
        {
            free(world->keybindings[key].name);
        }
        free(world->keybindings);
        free(world->keyswindata);
    }
    if (cleanup_flags & CLEANUP_LOG)
    {
        free(world->log);
    }
    if (cleanup_flags & CLEANUP_COMMAND_DB)
    {
        free_command_db(world);
    }
    if (cleanup_flags & CLEANUP_WIN_INFO)
    {
        free_win(world->wins.info);
    }
    if (cleanup_flags & CLEANUP_WIN_MAP)
    {
        free_win(world->wins.map);
    }
    if (cleanup_flags & CLEANUP_WIN_LOG)
    {
        free_win(world->wins.log);
    }
    if (cleanup_flags & CLEANUP_WIN_KEYS)
    {
        free_win(world->wins.keys);
    }
    if (cleanup_flags & CLEANUP_WIN_META)
    {
        delwin(world->wins.meta->padframe.curses_win);
        free(world->wins.meta);
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECT_DEFS)
    {
        free_item_defs(world->item_def);
        free_monster_defs(world->monster_def);
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECTS)
    {
        free_items(world->item);
        free_monsters(world->monster);
    }
}



extern void set_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags | flag;
}



extern void exit_game(struct World * world)
{
    cleanup(world);
    exit(EXIT_SUCCESS);
}



extern void exit_err(uint8_t err, struct World * world, char * msg)
{
    if (0 == err)
    {
        return;
    }
    cleanup(world);
    if (NULL == msg)
    {
        msg = "Details unknown.";
    }
    printf("Aborted PlomRogue due to error. %s\nInternal error code: %d\n",
           msg, err);
    if (0 != errno)
    {
        perror("errno states");
    }
    exit(EXIT_FAILURE);
}
