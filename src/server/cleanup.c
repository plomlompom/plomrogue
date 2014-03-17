/* src/server/cleanup.c */

#include "cleanup.h"
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* free() */
#include <unistd.h> /* unlink() */
#include "../common/readwrite.h" /* try_fclose() */
#include "map_object_actions.h" /* free_map_object_actions() */
#include "map_objects.h" /* free_map_objects(), free_map_object_defs() */
#include "world.h" /* global world */




/* The clean-up flags set by set_cleanup_flag(). */
static uint32_t cleanup_flags = 0x0000;



extern void cleanup()
{
    char * f_name = "cleanup()";
    free(world.queue);
    free(world.log);
    free(world.map.cells);
    if (cleanup_flags & CLEANUP_WORLDSTATE)
    {
        unlink(world.path_worldstate);
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
    if (cleanup_flags & CLEANUP_IN)
    {
        try_fclose(world.file_in, f_name);
        unlink(world.path_in);
    }
    if (cleanup_flags & CLEANUP_OUT)
    {
        try_fclose(world.file_out, f_name);
        free(world.server_test);
        unlink(world.path_out);
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
