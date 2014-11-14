/* src/server/cleanup.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "cleanup.h"
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* free() */
#include <unistd.h> /* unlink() */
#include "../common/readwrite.h" /* try_fclose() */
#include "hardcoded_strings.h" /* s */
#include "thing_actions.h" /* free_thing_actions() */
#include "things.h" /* free_things(), free_thing_types() */
#include "world.h" /* global world */




/* The clean-up flags set by set_cleanup_flag(). */
static uint32_t cleanup_flags = 0x0000;



extern void cleanup()
{
    free(world.queue);
    free(world.map.cells);
    if (cleanup_flags & CLEANUP_WORLDSTATE)
    {
        unlink(s[S_PATH_WORLDSTATE]);
    }
    if (cleanup_flags & CLEANUP_THINGS)
    {
        free_things(world.things);
    }
    if (cleanup_flags & CLEANUP_THING_TYPES)
    {
        free_thing_types(world.thing_types);
    }
    if (cleanup_flags & CLEANUP_THING_ACTIONS)
    {
        free_thing_actions(world.thing_actions);
    }
    if (cleanup_flags & CLEANUP_IN)
    {
        try_fclose(world.file_in, __func__);
        unlink(s[S_PATH_IN]);
    }
    if (cleanup_flags & CLEANUP_OUT)
    {
        try_fclose(world.file_out, __func__);
        free(world.server_test);
        unlink(s[S_PATH_OUT]);
    }
}


extern void set_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags | flag;
}



extern void unset_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags ^ flag;
}
