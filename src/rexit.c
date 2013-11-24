/* rexit.c */

#include "rexit.h"
#include <stdlib.h> /* for exit(), free(), defines EXIT_SUCESS, EXIT_FAILURE */
#include <stdio.h> /* for printf(), perror() */
#include <stdint.h> /* for uint8_t */
#include <errno.h> /* for errno */
#include "main.h" /* for world global */
#include "map.h" /* for Map struct */
#include "keybindings.h" /* for free_keybindings() */
#include "command_db.h" /* for free_command_db() */
#include "windows.h" /* for Win struct, free_winmeta_and_endwin() */
#include "map_objects.h" /* for free_map_objects, free_map_object_defs() */
#include "misc.h" /* for unload_interface_conf() */
#include "map_object_actions.h" /* for free_map_object_actions() */



/* The clean-up routine and the flag resource by which it decides what to do. */
static uint32_t cleanup_flags = 0x0000;
static void cleanup();



static void cleanup()
{
    if (cleanup_flags & CLEANUP_MAP_OBJECTS)
    {
        free_map_objects(world.map_objs);
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECT_DEFS)
    {
        free_map_object_defs(world.map_obj_defs);
    }
    if (cleanup_flags & CLEANUP_LOG)
    {
        free(world.log);
    }
    if (cleanup_flags & CLEANUP_COMMAND_DB)
    {
        free_command_db();
    }
    if (cleanup_flags & CLEANUP_MAPOBJACTS)
    {
        free_map_object_actions(world.map_obj_acts);
    }
    if (cleanup_flags & CLEANUP_MAP)
    {
        free(world.map->cells);
    }
    if (cleanup_flags & CLEANUP_INTERFACE)
    {
        unload_interface_conf();
    }
    if (cleanup_flags & CLEANUP_NCURSES)
    {
        free_winmeta_and_endwin();
    }
}



extern void set_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags | flag;
}



extern void exit_game()
{
    cleanup();
    exit(EXIT_SUCCESS);
}



extern void exit_err(uint8_t err, char * msg)
{
    if (0 == err)
    {
        return;
    }
    cleanup();
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
