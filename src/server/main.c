/* src/server/main.c */

#include <errno.h> /* global errno */
#include <stdio.h> /* printf() */
#include <stdlib.h> /* exit() */
#include <sys/stat.h> /* mkfifo(), mkdir() */
#include <unistd.h> /* access() */
#include "../common/err_try_fgets.h" /* set_err_try_fgets_delim() */
#include "../common/rexit.h" /* exit_err, exit_trouble(), set_cleanup_func() */
#include "cleanup.h" /* set_cleanup_flag(), cleanup() */
#include "init.h" /* run_game(), obey_argv() */
#include "map_object_actions.h" /* init_map_object_actions() */
#include "map_objects.h" /* init_map_object_defs() */
#include "run.h" /* obey_argv(), run_game() */
#include "world.h" /* struct World */



struct World world;



int main(int argc, char ** argv)
{
    char * f_name = "main()";

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
    world.path_map_obj_defs = "confserver/defs";
    world.path_map_obj_acts = "confserver/map_object_actions";
    world.path_in     = "server/in";
    world.path_out    = "server/out";
    world.path_record = "record";
    world.tmp_suffix  = "_tmp";
    set_err_try_fgets_delim("%%\n");

    /* Set map geometry. */
    world.map.size.x = 64;
    world.map.size.y = 64;
    world.map.dist_orthogonal = 5;
    world.map.dist_diagonal   = 7;

    /* Check existence of config files. */
    char * err_mod = "No map object definitions file.";
    char * err_moa = "No map object actions file.";
    exit_err(access(world.path_map_obj_defs, F_OK), err_mod);
    exit_err(access(world.path_map_obj_acts, F_OK), err_moa);

    /* Treat world.path_in file as server process lock file. */
    char * err = "Found pre-existing input fifo file. This indicates another "
                 "roguelike-server may be running. It should be killed first.";
    exit_err(!access(world.path_in, F_OK), err);
    int test = mkdir("server", 0700);
    exit_trouble(test && EEXIST != errno, f_name, "mkdir()");
    exit_trouble(mkfifo(world.path_in, 0600), f_name, "mkfifo()");
    set_cleanup_flag(CLEANUP_FIFO);

    /* Init from config files map object (action) definitions. */
    init_map_object_defs();
    init_map_object_actions();

    /* Enter play or replay mode loops, then leave properly. */
    run_game();
    cleanup();
    exit(EXIT_SUCCESS);
}
