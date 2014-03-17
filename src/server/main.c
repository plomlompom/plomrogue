/* src/server/main.c */

#include <errno.h> /* global errno */
#include <stdio.h> /* printf(), fflush() */
#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() */
#include <sys/stat.h> /* mkfifo(), mkdir() */
#include <sys/types.h> /* defines pid_t, time_t */
#include <time.h> /* #time() */
#include <unistd.h> /* access(), getpid(), unlink() */
#include "../common/err_try_fgets.h" /* set_err_try_fgets_delim() */
#include "../common/readwrite.h" /* try_fopen(), try_fwrite(), try_fclose() */
#include "../common/rexit.h" /* exit_err, set_cleanup_func() */
#include "../common/try_malloc.h" /* try_malloc() */
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
    world.path_worldstate   = "server/worldstate";
    world.path_out          = "server/out";
    world.path_in           = "server/in";
    world.path_record       = "record";
    world.tmp_suffix        = "_tmp";
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

    /* Init from config files map object (action) definitions. */
    init_map_object_defs();
    init_map_object_actions();

    /* Create server directory if it does not exist yet. */
    int test = mkdir("server", 0700);
    exit_trouble(test && EEXIST != errno, f_name, "mkdir()");

    /* Create server out file and start it with server process test string. */
    world.file_out = try_fopen(world.path_out, "w", f_name);
    world.server_test = try_malloc(10 + 1 + 10 + 1 + 1, f_name);
    sprintf(world.server_test, "%d %d\n", getpid(), (int) time(0));
    try_fwrite(world.server_test, strlen(world.server_test), 1,
               world.file_out, f_name);
    fflush(world.file_out);
    set_cleanup_flag(CLEANUP_OUT);

    /* Create server in file, switch to reading it. */
    if (!access(world.path_in, F_OK))  /* This keeps out input from old input */
    {                                  /* file streams of clients             */
        unlink(world.path_in);         /* communicating with server processes */
    }                                  /* superseded by this current one.     */
    world.file_in = try_fopen(world.path_in, "w", f_name);
    try_fclose(world.file_in, f_name);
    world.file_in = try_fopen(world.path_in, "r", f_name);
    set_cleanup_flag(CLEANUP_IN);

    /* Enter play or replay mode loops, then leave properly. */
    run_game();
    cleanup();
    exit(EXIT_SUCCESS);
}
