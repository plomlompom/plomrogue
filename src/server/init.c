/* src/server/init.c */

#include "init.h"
#include <errno.h> /* global errno, EEXIST */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint32_t */
#include <stdio.h> /* sprintf(), fflush() */
#include <stdlib.h> /* exit(), free() */
#include <string.h> /* atoi(), strlen() */
#include <sys/stat.h> /* mkdir() */
#include <sys/types.h> /* defines pid_t, time_t */
#include <time.h> /* time() */
#include <unistd.h> /* optarg, getopt(), access(), unlink(), getpid() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), textfile_width(),
                                  * try_fgets(), try_fwrite()
                                  */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag(), CLEANUP_MAP_OBJ_DEFS,
                      * CLEANUP_MAP_OBJ_ACTS
                      */
#include "io.h" /* read_config_file(), struct EntrySkeleton */
#include "map.h" /* init_map() */
#include "map_object_actions.h" /* struct MapObjAct, read_map_object_action() */
#include "map_objects.h" /* struct MapObjDef, free_map_objects(),
                          * add_map_objects(), read_map_object_def()
                          */
#include "rrand.h" /* rrand() */
#include "run.h" /* obey_msg(), io_loop() */
#include "world.h" /* global world */



extern void obey_argv(int argc, char * argv[])
{
    int opt;
    while (-1 != (opt = getopt(argc, argv, "vs::")))
    {
        if      ('v' == opt)
        {
            world.is_verbose = 1;
        }
        else if ('s' == opt)
        {
            world.replay = 1;
            if (optarg)
            {
                world.replay = atoi(optarg);
            }
        }
        else
        {
            exit(EXIT_FAILURE);
        }
    }
}



extern void init_map_and_map_objects_configs()
{
    world.map.size.x = 64;
    world.map.size.y = 64;
    world.map.dist_orthogonal = 5;
    world.map.dist_diagonal   = 7;
    char * err_mod = "No map object definitions file.";
    char * err_moa = "No map object actions file.";
    exit_err(access(world.path_map_obj_defs, F_OK), err_mod);
    exit_err(access(world.path_map_obj_acts, F_OK), err_moa);
    read_config_file(world.path_map_obj_defs, CLEANUP_MAP_OBJECT_DEFS,
                     read_map_object_def, sizeof(struct MapObjDef),
                     (struct EntrySkeleton **) &world.map_obj_defs);
    read_config_file(world.path_map_obj_acts, CLEANUP_MAP_OBJECT_ACTS,
                     read_map_object_action, sizeof(struct MapObjAct),
                     (struct EntrySkeleton **) &world.map_obj_acts);
}



extern void setup_server_io()
{
    char * f_name = "setup_server_io()";
    int test = mkdir("server", 0700);
    exit_trouble(test && EEXIST != errno, f_name, "mkdir()");
    world.file_out = try_fopen(world.path_out, "w", f_name);
    world.server_test = try_malloc(10 + 1 + 10 + 1 + 1, f_name);
    sprintf(world.server_test, "%d %d\n", getpid(), (int) time(0));
    try_fwrite(world.server_test, strlen(world.server_test), 1,
               world.file_out, f_name);
    fflush(world.file_out);
    set_cleanup_flag(CLEANUP_OUT);
    if (!access(world.path_in, F_OK))  /* This keeps out input from old input */
    {                                  /* file streams of clients             */
        unlink(world.path_in);         /* communicating with server processes */
    }                                  /* superseded by this current one.     */
    world.file_in = try_fopen(world.path_in, "w", f_name);
    try_fclose(world.file_in, f_name);
    world.file_in = try_fopen(world.path_in, "r", f_name);
    set_cleanup_flag(CLEANUP_IN);
}



extern void remake_world(uint32_t seed)
{
    char * f_name = "remake_world()";
    free(world.log);
    world.log = NULL;  /* map_object_action.c's update_log() checks for this. */
    world.seed = seed;
    world.map_obj_count = 0;
    free(world.map.cells);
    free_map_objects(world.map_objs);
    world.last_update_turn = 0;
    init_map();
    add_map_objects(0, 1);
    add_map_objects(1, 1 + rrand() % 27);
    add_map_objects(2, 1 + rrand() % 9);
    add_map_objects(3, 1 + rrand() % 3);
    add_map_objects(4, 1 + rrand() % 3);
    add_map_objects(5, 1 + rrand() % 3);
    set_cleanup_flag(CLEANUP_MAP_OBJECTS);
    if (world.turn)
    {
        exit_trouble(unlink(world.path_record), f_name, "unlink()");
    }
    world.turn = 1;
}



extern void run_game()
{
    char * f_name = "run_game()";
    if (!access(world.path_record, F_OK))
    {
        FILE * file = try_fopen(world.path_record, "r", f_name);
        uint32_t linemax = textfile_width(file);
        char line[linemax + 1];
        while (   (!world.replay || (world.turn < world.replay))
               && NULL != try_fgets(line, linemax + 1, file, f_name))
        {
            obey_msg(line, 0);
        }
        if (!world.replay)
        {
            try_fclose(file, f_name);
            io_loop();
            return;
        }
        uint8_t end = 0;
        while (!io_loop())
        {
            if (!end)
            {
                end = (NULL == try_fgets(line, linemax + 1, file, f_name));
                if (!end)
                {
                    obey_msg(line, 0);
                }
            }
        }
        try_fclose(file, f_name);
        return;
    }
    exit_err(world.replay, "No record file found to replay.");
    char * command = "seed";
    char msg[strlen(command) + 1 + 11 + 1];
    sprintf(msg, "%s %d", command, (int) time(NULL));
    obey_msg(msg, 1);
    io_loop();
}
