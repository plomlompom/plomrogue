/* src/server/init.c */

#define _POSIX_C_SOURCE 2 /* getopt(), optarg */
#include "init.h"
#include <errno.h> /* global errno, EEXIST */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint32_t */
#include <stdio.h> /* FILE, sprintf(), fflush() */
#include <stdlib.h> /* exit(), free(), atoi() */
#include <string.h> /* strlen() */
#include <sys/stat.h> /* mkdir() */
#include <sys/types.h> /* defines pid_t, time_t */
#include <time.h> /* time() */
#include <unistd.h> /* optarg, getopt(), access(), unlink(), getpid() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), textfile_width(),
                                  * try_fgets(), try_fwrite()
                                  */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "field_of_view.h" /* build_fov_map() */
#include "hardcoded_strings.h" /* s */
#include "map.h" /* remake_map() */
#include "things.h" /* Thing, ThingType, free_things(), add_things(),
                     * get_player()
                     */
#include "run.h" /* obey_msg(), io_loop() */
#include "world.h" /* global world */




/* Replay game from record file up to the turn named in world.replay, then turn
 * over to manual replay via io_loop().
 */
static void replay_game();



static void replay_game()
{
    char * f_name = "replay_game()";
    exit_err(access(s[PATH_RECORD], F_OK), "No record found to replay.");
    FILE * file = try_fopen(s[PATH_RECORD], "r", f_name);
    uint32_t linemax = textfile_width(file);
    char line[linemax + 1];
    while (   world.turn < world.replay
           && NULL != try_fgets(line, linemax + 1, file, f_name))
    {
        obey_msg(line, 0);
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
}



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



extern void setup_server_io()
{
    char * f_name = "setup_server_io()";
    int test = mkdir("server", 0700);
    exit_trouble(test && EEXIST != errno, f_name, "mkdir()");
    world.file_out = try_fopen(s[PATH_OUT], "w", f_name);
    world.server_test = try_malloc(10 + 1 + 10 + 1 + 1, f_name);
    sprintf(world.server_test, "%d %d\n", getpid(), (int) time(0));
    try_fwrite(world.server_test, strlen(world.server_test), 1,
               world.file_out, f_name);
    fflush(world.file_out);
    set_cleanup_flag(CLEANUP_OUT);
    char * path_in = s[PATH_IN];
    if (!access(path_in, F_OK))        /* This keeps out input from old input */
    {                                  /* file streams of clients             */
        unlink(path_in)   ;            /* communicating with server processes */
    }                                  /* superseded by this current one.     */
    world.file_in = try_fopen(path_in, "w", f_name);
    try_fclose(world.file_in, f_name);
    world.file_in = try_fopen(path_in, "r", f_name);
    set_cleanup_flag(CLEANUP_IN);
}



extern void remake_world()
{
    char * f_name = "remake_world()";
    free(world.log);
    world.log = NULL;      /* thing_actions.c's update_log() checks for this. */
    world.seed_map = world.seed;
    free_things(world.things);
    world.last_update_turn = 0;
    remake_map();
    struct ThingType * tt;
    for (tt = world.thing_types; NULL != tt; tt = tt->next)
    {
        if (world.player_type == tt->id)
        {
            add_things(tt->id, tt->start_n);
            break;
        }
    }
    for (tt = world.thing_types; NULL != tt; tt = tt->next)
    {
        if (world.player_type != tt->id)
        {
            add_things(tt->id, tt->start_n);
        }
    }
    set_cleanup_flag(CLEANUP_THINGS);
    struct Thing * t;
    for (t = world.things; NULL != t; t = t->next)
    {
        t->fov_map = t->lifepoints ? build_fov_map(t) : NULL;
    }
    if (!world.replay && !access(s[PATH_RECORD], F_OK))
    {
        exit_trouble(unlink(s[PATH_RECORD]), f_name, "unlink()");
    }
    world.turn = 1;
}



extern void run_game()
{
    char * f_name = "run_game()";
    if (world.replay)
    {
        replay_game();
        return;
    }
    char * path_savefile = s[PATH_SAVE];
    if (!access(path_savefile, F_OK))
    {
        FILE * file = try_fopen(path_savefile, "r", f_name);
        uint32_t linemax = textfile_width(file);
        char line[linemax + 1];
        while (NULL != try_fgets(line, linemax + 1, file, f_name))
        {
            if (strlen(line) && strcmp("\n", line))
            {
                obey_msg(line, 0);
            }
        }
        try_fclose(file, f_name);
    }
    else
    {
        char * command = s[CMD_MAKE_WORLD];
        char msg[strlen(command) + 1 + 11 + 1];
        sprintf(msg, "%s %d", command, (int) time(NULL));
        obey_msg(msg, 1);
    }
    io_loop();
}
