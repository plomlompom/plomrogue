/* src/client/cleanup.c */

#include "cleanup.h"
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* free() */
#include "command_db.h" /* free_command_db() */
#include "misc.h" /* unload_interface_conf() */
#include "windows.h" /* free_winmeta_and_endwin() */
#include "world.h" /* world global */



/* The clean-up flags set by set_cleanup_flag(). */
static uint32_t cleanup_flags = 0x0000;



extern void cleanup()
{
    free(world.map.cells);
    free(world.log);
    free(world.player_inventory);
    if (cleanup_flags & CLEANUP_INTERFACE)/* Must come pre ncurses cleanup,   */
    {                                     /* for by closing all windows it    */
        unload_interface_conf();          /* relies on world.wmeta data freed */
    }                                     /* by free_winmeta_and_endwin().    */
    if (cleanup_flags & CLEANUP_NCURSES)
    {
        free_winmeta_and_endwin();
    }
    if (cleanup_flags & CLEANUP_COMMANDS)
    {
        free_command_db();
    }
}


extern void set_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags | flag;
}
