/* rexit.c */

#include "rexit.h"
#include <stdlib.h> /* for exit(), free(), defines EXIT_SUCESS, EXIT_FAILURE */
#include <stdio.h> /* for printf(), perror() */
#include <stdint.h> /* for uint8_t */
#include <ncurses.h> /* for endwin() */
#include <errno.h> /* for errno */
#include "main.h" /* for World struct */
#include "map.h" /* for Map struct */
#include "keybindings.h" /* for free_keybindings() */
#include "command_db.h" /* for free_command_db() */
#include "windows.h" /* for Win struct, free_win(), free_winmeta() */
#include "map_objects.h" /* for free_item_defs(), free_monster_defs() */
#include "wincontrol.h" /* for free_winconfs() */
#include "misc.h" /* for unload_interface_conf() */



/* The clean-up routine and the flag resource by which it decides what to do. */
static uint32_t cleanup_flags = 0x0000;
static void cleanup(struct World * world);



static void cleanup(struct World * world)
{
    if (cleanup_flags & CLEANUP_NCURSES)
    {
        endwin();
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECTS)
    {
        free_items(world->item);
        free_monsters(world->monster);
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECT_DEFS)
    {
        free_item_defs(world->item_def);
        free_monster_defs(world->monster_def);
    }
    if (cleanup_flags & CLEANUP_LOG)
    {
        free(world->log);
    }
    if (cleanup_flags & CLEANUP_COMMAND_DB)
    {
        free_command_db(world);
    }
    if (cleanup_flags & CLEANUP_MAP)
    {
        free(world->map->cells);
    }
    if (cleanup_flags & CLEANUP_INTERFACE_CONF)
    {
        unload_interface_conf(world);
    }
    if (cleanup_flags & CLEANUP_WIN_META)
    {
        free_winmeta(world->wmeta);
    }
/*
    if (cleanup_flags & CLEANUP_KEYBINDINGS)
    {
        free_keybindings(world->kb_global.kbs);
        free_keybindings(world->kb_wingeom.kbs);
        free_keybindings(world->kb_winkeys.kbs);
    }
    if (cleanup_flags & CLEANUP_WINCONFS)
    {
        free_winconfs(world);
    }
*/
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
