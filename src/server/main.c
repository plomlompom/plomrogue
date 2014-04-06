/* src/server/main.c */

#include <stdio.h> /* printf() */
#include <stdlib.h> /* exit() */
#include "../common/rexit.h" /* exit_err, set_cleanup_func() */
#include "cleanup.h" /* set_cleanup_flag(), cleanup() */
#include "init.h" /* run_game(), obey_argv(), obey_argv(), setup_server_io(),
                   * init_map_and_map_object_configs()
                   */
#include "world.h" /* struct World */



struct World world;



int main(int argc, char ** argv)
{
    /* So error exits also go through the server's cleanup() function. */
    set_cleanup_func(cleanup);

    /* Init settings from command line / hard-coded values. Print start info. */
    obey_argv(argc, argv);
    if (world.is_verbose)
    {
        char * printf_err = "Trouble in main() with printf().";
        int test = printf("Starting plomrogue-server.\n");
        exit_err(-1 == test, printf_err);
        if (world.replay)
        {
            test = printf("Replay mode. Auto-replaying up to turn %d.\n",
                         world.replay);
            exit_err(-1 == test, printf_err);
        }
    }
    world.path_config       = "confserver/world";
    world.path_worldstate   = "server/worldstate";
    world.path_out          = "server/out";
    world.path_in           = "server/in";
    world.path_record       = "record";
    world.tmp_suffix        = "_tmp";

    /* Init map, map object configurations and server i/o files. */
    init_map_and_map_objects_configs();
    setup_server_io();

    /* Enter play or replay mode loops, then leave properly. */
    run_game();
    cleanup();
    exit(EXIT_SUCCESS);
}
