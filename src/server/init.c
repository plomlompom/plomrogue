/* src/server/init.c */

#include "init.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* exit(), free() */
#include <string.h> /* atoi() */
#include <time.h> /* time() */
#include <unistd.h> /* optarg, getopt(), access(), unlink() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), textfile_width(),
                                  * try_fgets()
                                  */
#include "../common/rexit.h" /* exit_err() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "map_objects.h" /* free_map_objects(), add_map_objects() */
#include "map.h" /* init_map() */
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
