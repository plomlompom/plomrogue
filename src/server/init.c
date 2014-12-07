/* src/server/init.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

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
#include <unistd.h> /* optarg, getopt(), access(), getpid() */
#include "../common/parse_file.h" /* err_line_zero(), err_line_inc() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), textfile_width(),
                                  * try_fgets(), try_fwrite(),
                                  * detect_atomic_leftover()
                                  */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "field_of_view.h" /* build_fov_map() */
#include "hardcoded_strings.h" /* s */
#include "map.h" /* remake_map() */
#include "things.h" /* Thing, ThingType, free_things(), add_things(),
                     * get_thing_id_action_id_by_name()
                     */
#include "run.h" /* obey_msg(), io_loop(), record(), send_to_outfile() */
#include "world.h" /* global world */




/* Pass to obey_msg() lines from file at "path", on "record" write to same. Do
 * not pass lines that consist only of a newline character. Transform newline
 * in the line passed to \0.
 */
static void obey_lines_from_file(char * path, uint8_t record);

/* Replay game from record file up to the turn named in world.replay, then turn
 * over to manual replay via io_loop().
 */
static void replay_game();

/* Return 1 if the type defined by world.player_type has a .start_n of 0.
 * Return 2 if no thing action with .name of s[S_CMD_WAIT] is defined.
 * Else, return 0.
 */
static uint8_t world_cannot_be_made();


static void obey_lines_from_file(char * path, uint8_t record)
{
    FILE * file = try_fopen(path, "r", __func__);
    uint32_t linemax = textfile_width(file);
    char * line = try_malloc(linemax + 1, __func__);
    while (try_fgets(line, linemax + 1, file, __func__))
    {
        if (strlen(line))
        {
            if (strcmp("\n", line))
            {
                char * nl = strchr(line, '\n');
                if (nl)
                {
                    *nl = '\0';
                }
                obey_msg(line, record);
            }
            err_line_inc();
        }
    }
    free(line);
    try_fclose(file, __func__);
}



static void replay_game()
{
    exit_err(access(s[S_PATH_RECORD], F_OK), "No record found to replay.");
    FILE * file = try_fopen(s[S_PATH_RECORD], "r", __func__);
    uint32_t linemax = textfile_width(file);
    char * line = try_malloc(linemax + 1, __func__);
    while (   world.turn < world.replay
           && try_fgets(line, linemax + 1, file, __func__))
    {
        obey_msg(line, 0);
        err_line_inc();
    }
    uint8_t end = 0;
    while (3 == io_loop(2))
    {
        if (!end)
        {
            end = (NULL == try_fgets(line, linemax + 1, file, __func__));
            if (!end)
            {
                obey_msg(line, 0);
                err_line_inc();
            }
        }
    }
    free(line);
    try_fclose(file, __func__);
}



static uint8_t world_cannot_be_made()
{
    uint8_t player_will_be_generated = 0;
    struct ThingType * tt;
    for (tt = world.thing_types; tt; tt = tt->next)
    {
        if (world.player_type == tt->id)
        {
            player_will_be_generated = 0 < tt->start_n;
            break;
        }
    }
    if (!player_will_be_generated)
    {
        return 1;
    }
    if (!get_thing_action_id_by_name(s[S_CMD_WAIT]))
    {
        return 2;
    }
    return 0;
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
    int test = mkdir("server", 0700);
    exit_trouble(test && EEXIST != errno, __func__, "mkdir");
    world.file_out = try_fopen(s[S_PATH_OUT], "w", __func__);
    world.server_test = try_malloc(10 + 1 + 10 + 1 + 1, __func__);
    test = sprintf(world.server_test, "%d %d\n", getpid(), (int) time(0));
    exit_trouble(test < 0, __func__, s[S_FCN_SPRINTF]);
    try_fwrite(world.server_test, strlen(world.server_test), 1,
               world.file_out, __func__);
    fflush(world.file_out);
    set_cleanup_flag(CLEANUP_OUT);
    char * path_in = s[S_PATH_IN];
    if (!access(path_in, F_OK))        /* This keeps out input from old input */
    {                                  /* file streams of clients             */
        unlink(path_in)   ;            /* communicating with server processes */
    }                                  /* superseded by this current one.     */
    world.file_in = try_fopen(path_in, "w", __func__);
    try_fclose(world.file_in, __func__);
    world.file_in = try_fopen(path_in, "r", __func__);
    set_cleanup_flag(CLEANUP_IN);
}



extern uint8_t remake_world()
{
    uint8_t test = world_cannot_be_made();
    if (test)
    {
        return test;
    }
    world.seed_map = world.seed;
    free_things(world.things);
    remake_map();
    struct ThingType * tt;
    for (tt = world.thing_types; tt; tt = tt->next)
    {
        if (world.player_type == tt->id)
        {
            add_things(tt->id, tt->start_n);
            break;
        }
    }
    for (tt = world.thing_types; tt; tt = tt->next)
    {
        if (world.player_type != tt->id)
        {
            add_things(tt->id, tt->start_n);
        }
    }
    struct Thing * t;
    for (t = world.things; t; t = t->next)
    {
        if (t->lifepoints)
        {
            build_fov_map(t);
        }
    }
    world.turn = 1;
    world.exists = 1;
    send_to_outfile("NEW_WORLD\n", 1);
    return 0;
}



extern void run_game()
{
    detect_atomic_leftover(s[S_PATH_SAVE]);
    detect_atomic_leftover(s[S_PATH_RECORD]);
    err_line_zero();
    if (world.replay)
    {
        replay_game();
        return;
    }
    if (!access(s[S_PATH_SAVE], F_OK))
    {
        obey_lines_from_file(s[S_PATH_SAVE], 0);
    }
    else
    {
        char * err = "No world config file from which to start a new world.";
        exit_err(access(s[S_PATH_CONFIG], F_OK), err);
        obey_lines_from_file(s[S_PATH_CONFIG], 1);
        err_line_zero();
        char * command = s[S_CMD_MAKE_WORLD];
        char * msg = try_malloc(strlen(command) + 1 + 11 + 1, __func__);
        int test = sprintf(msg, "%s %d", command, (int) time(NULL));
        exit_trouble(test < 0, __func__, s[S_FCN_SPRINTF]);
        obey_msg(msg, 1);
        free(msg);
    }
    err_line_zero();
    io_loop(1);
    record(NULL, 1);
}
