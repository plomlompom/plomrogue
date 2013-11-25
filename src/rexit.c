/* rexit.c */

#include "rexit.h"
#include <stdlib.h> /* for exit(), free(), defines EXIT_SUCESS, EXIT_FAILURE */
#include <stdio.h> /* for printf(), perror(), sprintf() */
#include <stdint.h> /* for uint8_t */
#include <string.h> /* for strlen() */
#include <errno.h> /* for errno */
#include "main.h" /* for world global */
#include "map.h" /* for Map struct ("free(world.map->cells)") */
#include "command_db.h" /* for free_command_db() */
#include "windows.h" /* for free_winmeta_and_endwin() */
#include "map_objects.h" /* for free_map_objects, free_map_object_defs() */
#include "misc.h" /* for unload_interface_conf() */
#include "map_object_actions.h" /* for free_map_object_actions() */



/* The clean-up routine and the flag resource by which it decides what to do. */
static uint32_t cleanup_flags = 0x0000;
static void cleanup();



static void cleanup()
{
    if (cleanup_flags & CLEANUP_LOG)
    {
        free(world.log);
    }
    if (cleanup_flags & CLEANUP_COMMAND_DB)
    {
        free_command_db();
    }
    if (cleanup_flags & CLEANUP_MAP)
    {
        free(world.map->cells);
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECTS)
    {
        free_map_objects(world.map_objs);
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECT_DEFS)
    {
        free_map_object_defs(world.map_obj_defs);
    }
    if (cleanup_flags & CLEANUP_MAP_OBJECT_ACTS)
    {
        free_map_object_actions(world.map_obj_acts);
    }
    if (cleanup_flags & CLEANUP_INTERFACE) /* Only cleaning-up order          */
    {                                      /* dependency known so far:        */
        unload_interface_conf();           /* unload_interface_conf() must    */
    }                                      /* come before                     */
    if (cleanup_flags & CLEANUP_NCURSES)   /* free_winmeta_and_endwin()       */
    {                                      /* since it depends on world.wmeta */
        free_winmeta_and_endwin();         /* for closing all windows.        */
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



extern void exit_trouble(uint8_t err, char * parent, char * child)
{
    char * p1 = "Trouble in ";
    char * p2 = " with ";
    char * p3 = ".";
    uint16_t size = strlen(p1) + strlen(parent) + strlen(p2) + strlen(child)
                    + strlen(p3) + 1;
    char msg[size];
    sprintf(msg, "%s%s%s%s%s", p1, parent, p2, child, p3);
    exit_err(err, msg);
}
