/* src/client/cleanup.c */

#include "cleanup.h"
#include <ncurses.h> /* for endwin() */
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* free() */
#include "../common/readwrite.h" /* try_fclose() */
#include "command_db.h" /* free_command_db() */
#include "interface_conf.h" /* unload_interface_conf() */
#include "world.h" /* world global */



/* The clean-up flags set by set_cleanup_flag(). */
static uint32_t cleanup_flags = 0x0000;



extern void cleanup()
{
    char * f_name = "cleanup()";
    free(world.map.cells);
    free(world.log);
    free(world.player_inventory);
    if (cleanup_flags & CLEANUP_INTERFACE)
    {
        unload_interface_conf();
    }
    if (cleanup_flags & CLEANUP_NCURSES)
    {
        endwin();
    }
    if (cleanup_flags & CLEANUP_COMMANDS)
    {
        free_command_db();
    }
    if (cleanup_flags & CLEANUP_SERVER_IN)
    {
        try_fclose(world.file_server_in, f_name);
    }
    if (cleanup_flags & CLEANUP_SERVER_OUT)
    {
        try_fclose(world.file_server_out, f_name);
    }
}


extern void set_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags | flag;
}
