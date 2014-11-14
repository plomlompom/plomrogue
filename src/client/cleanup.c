/* src/client/cleanup.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

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
    free(world.map.cells);
    free(world.mem_map);
    free(world.log);
    free(world.queue);
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
        try_fclose(world.file_server_in, __func__);
    }
    if (cleanup_flags & CLEANUP_SERVER_OUT)
    {
        try_fclose(world.file_server_out, __func__);
    }
}


extern void set_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags | flag;
}
