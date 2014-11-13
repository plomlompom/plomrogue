/* src/server/run.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "run.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, int16_t */
#include <stdio.h> /* FILE, printf(), fflush() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), strcmp(), strncmp(), strdup() */
#include <time.h> /* time_t, time() */
#include <unistd.h> /* access() */
#include "../common/parse_file.h" /* set_err_line_options(), token_from_line(),
                                   * err_line(), err_line_inc(), parse_val()
                                   */
#include "../common/readwrite.h" /* try_fopen(), try_fcose(), try_fwrite(),
                                  * try_fgets(), textfile_width(), try_fputc(),
                                  * atomic_write_finish(), build_temp_path()
                                  */
#include "../common/rexit.h" /* exit_trouble(), exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "ai.h" /* ai() */
#include "cleanup.h" /* unset_cleanup_flag() */
#include "god_commands.h" /* parse_god_command_(1|2|3)arg() */
#include "hardcoded_strings.h" /* s */
#include "io.h" /* io_round(), save_world() */
#include "things.h" /* Thing, ThingType, get_thing_action_id_by_name(),
                     * get_player(), try_thing_proliferation()
                     */
#include "world.h" /* world */



/* If "string" and "comparand" match in string, set "c_to_set" to value."  */
static uint8_t set_char_by_string_comparison(char * string, char * comparand,
                                             char * c_to_set, char value);

/* Return 1 on world.exists, else 0 and err_line() appropriate error message. */
static uint8_t player_commands_allowed();

/* Parse player command "tok0" with no argument to player action. */
static uint8_t parse_player_command_0arg(char * tok0);

/* Parse player command "tok0" with one argument "tok1" to player action. */
static uint8_t parse_player_command_1arg(char * tok0, char * tok1);

/* Parse/apply command "tok0" (read further tokens as necessary). */
static uint8_t parse_command(char * tok0);

/* Compares first line of server out file to world.server_test, aborts if they
 * don't match, but not before unsetting the flags deleting files in the server
 * directory, for in that case those must be assumed to belong to another server
 * process.
 */
static void server_test();

/* Return array of IDs of non-owned things in game world, ended by non-ID -1. */
static int16_t * build_whitelist();

/* Return 1 if value of "id" appears in "whitelist", else 0. */
static uint8_t thing_in_whitelist(uint8_t id, int16_t * whitelist);

/* Run the game world and its inhabitants (and their actions) until the player
 * avatar is free to receive new commands (or is dead). Only actions and
 * proliferations for non-owned things are performed that exist at the start of
 * the turn jumped into, or started anew by the cycle.
 */
static void turn_over();

/* Append "answer" to server output file, with instant fflush(). */
static void answer_query(char * answer);

/* Try to read "msg" as meta command, act accordingly; on success, free it. */
static uint8_t meta_commands(char * msg);



static uint8_t set_char_by_string_comparison(char * string, char * comparand,
                                             char * c_to_set, char value)
{
    if (!strcmp(string, comparand))
    {
        * c_to_set = value;
        return 1;
    }
    return 0;
}



static uint8_t player_commands_allowed()
{
    if (!world.exists)
    {
        return !err_line(1, "No world exists in which to run player commands.");
    }
    return 1;
}



static uint8_t parse_player_command_0arg(char * tok0)
{
    struct Thing * player = get_player();
    if (   !strcmp(tok0, s[S_CMD_WAIT]) || !strcmp(tok0, s[S_CMD_PICKUP])
        || !strcmp(tok0, s[S_CMD_AI]))
    {
        if (player_commands_allowed())
        {
            if (!strcmp(tok0, s[S_CMD_AI]))
            {
                ai(player);
            }
            else
            {
                player->command = get_thing_action_id_by_name(tok0);
                player->arg = 0;
            }
            turn_over();
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_player_command_1arg(char * tok0, char * tok1)
{
    struct Thing * player = get_player();
    if (   (   parse_val(tok0, tok1, s[S_CMD_DROP], '8', (char *) &player->arg)
            || parse_val(tok0, tok1, s[S_CMD_USE], '8', (char *) &player->arg))
        && player_commands_allowed())
    {
        player->command = get_thing_action_id_by_name(tok0);
        turn_over();
    }
    else if (!strcmp(tok0, s[S_CMD_MOVE]) && player_commands_allowed())
    {
        char dir = '\0';
        if (!(   set_char_by_string_comparison(tok1, "east",       &dir, 'd')
              || set_char_by_string_comparison(tok1, "south-east", &dir, 'c')
              || set_char_by_string_comparison(tok1, "south-west", &dir, 'x')
              || set_char_by_string_comparison(tok1, "west",       &dir, 's')
              || set_char_by_string_comparison(tok1, "north-west", &dir, 'w')
              || set_char_by_string_comparison(tok1, "north-east", &dir, 'e')))
        {
            return 0;
        }
        player->arg = dir;
        player->command = get_thing_action_id_by_name(tok0);
        turn_over();
    }
    else
    {
        return 0;
    }
    return 1;
}



static uint8_t parse_command(char * tok0)
{
    if (parse_player_command_0arg(tok0))
    {
        return 1;
    }
    else
    {
        char * tok1 = token_from_line(NULL);
        if (tok1 && (   parse_player_command_1arg(tok0, tok1)
                     || parse_god_command_1arg(tok0, tok1)))
        {
            return 1;
        }
        else
        {
            char * tok2 = token_from_line(NULL);
            if (tok2 && parse_god_command_2arg(tok0, tok1, tok2))
            {
                return 1;
            }
            else
            {
                char * tok3 = token_from_line(NULL);
                if (tok2 && parse_god_command_3arg(tok0, tok1, tok2, tok3))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}



static void server_test()
{
    char test[10 + 1 + 10 + 1 + 1];
    FILE * file = try_fopen(s[S_PATH_OUT], "r", __func__);
    try_fgets(test, 10 + 10 + 1 + 1, file, __func__);
    try_fclose(file, __func__);
    if (strcmp(test, world.server_test))
    {
        unset_cleanup_flag(CLEANUP_WORLDSTATE);
        unset_cleanup_flag(CLEANUP_OUT);
        unset_cleanup_flag(CLEANUP_IN);
        char * msg = "Server test string in server output file does not match. "
                     "This indicates that the current server process has been "
                     "superseded by another one.";
        exit_err(1, msg);
    }
}



static int16_t * build_whitelist()
{
    uint16_t i_things = NULL != world.things;
    struct Thing * t = world.things;
    for (; t; t = t->next, i_things++);
    int16_t * whitelist = try_malloc(i_things * sizeof(int16_t), __func__);
    for (i_things = 0, t = world.things; t;
         whitelist[i_things] = t->id, t = t->next, i_things++);
    whitelist[i_things] = -1;
    return whitelist;
}



static uint8_t thing_in_whitelist(uint8_t id, int16_t * whitelist)
{
    uint16_t i;
    for (i = 0; -1 < whitelist[i]; i++)
    {
        if ((int16_t) id == whitelist[i])
        {
            return 1;
        }
    }
    return 0;
}



static void turn_over()
{
    struct Thing * player = get_player();
    struct Thing * thing = player;
    int16_t * whitelist = build_whitelist();
    while (0 < player->lifepoints)
    {
        if (!thing)
        {
            world.turn++;
            thing = world.things;
            free(whitelist);
            whitelist = build_whitelist();/* The whitelist excludes things    */
        }                                 /* that appear only during the turn.*/
        if (thing_in_whitelist(thing->id, whitelist))
        {
            if (0 < thing->lifepoints)
            {
                if (0 == thing->command)
                {
                    if (thing == player)
                    {
                        break;
                    }
                    ai(thing);
                }
                thing->progress++;
                struct ThingAction * ta = get_thing_action(thing->command);
                if (thing->progress == ta->effort)
                {
                    ta->func(thing);
                    thing->command = 0;
                    thing->progress = 0;
                }
            }
            try_thing_proliferation(thing);
        }
        thing = thing->next;
    }
    free(whitelist);
}



static void answer_query(char * answer)
{
    try_fwrite(answer, strlen(answer), 1, world.file_out, __func__);
    fflush(world.file_out);
}



static uint8_t meta_commands(char * msg)
{
    if (!strcmp("QUIT", msg))
    {
        free(msg);
        return 2;
    }
    if (!strcmp("PING", msg))
    {
        free(msg);
        answer_query("PONG\n");
        return 1;
    }
    if (!strcmp("STACK", msg))
    {
        free(msg);
        answer_query("THINGS_BELOW_PLAYER START\n");
        struct Thing * player = get_player();
        struct Thing * t;
        for (t = world.things; t; t = t->next)
        {
            if (   t->pos.y == player->pos.y && t->pos.x == player->pos.x
                && t != player)
            {
                struct ThingType * tt = get_thing_type(t->type);
                answer_query(tt->name);
                answer_query("\n");
            }
        }
        answer_query("THINGS_BELOW_PLAYER END\n");
        return 1;
    }
    return 0;
}



extern void record(char * msg, uint8_t force)
{
    static FILE * file_tmp = NULL;
    static time_t save_wait = 0;
    static char * path_tmp;
    if (!file_tmp)
    {
        path_tmp = build_temp_path(s[S_PATH_RECORD]);
        file_tmp = try_fopen(path_tmp, "w", __func__);
        if (!access(s[S_PATH_RECORD], F_OK))
        {
            FILE * file_read = try_fopen(s[S_PATH_RECORD], "r", __func__);
            uint32_t linemax = textfile_width(file_read);
            char * line = try_malloc(linemax + 1, __func__);
            while (try_fgets(line, linemax + 1, file_read, __func__))
            {
                try_fwrite(line, strlen(line), 1, file_tmp, __func__);
            }
            free(line);
            try_fclose(file_read, __func__);
        }
    }
    if (msg)
    {
        try_fwrite(msg, strlen(msg), 1, file_tmp, __func__);
        try_fputc('\n', file_tmp, __func__);
    }
    if (force || time(NULL) > save_wait + 15)
    {
        save_wait = time(NULL);
        save_world();
        atomic_write_finish(file_tmp, s[S_PATH_RECORD], path_tmp);
        file_tmp = NULL;
    }
}



extern void obey_msg(char * msg, uint8_t do_record, uint8_t do_verbose)
{
    if (world.is_verbose && do_verbose)
    {
        exit_trouble(-1 == printf("Input: %s\n", msg), __func__, "printf");
    }
    set_err_line_options("Trouble with message: ", msg, 0);
    char * msg_copy = strdup(msg);
    char * tok0 = token_from_line(msg_copy);
    if (tok0)
    {

        if (parse_command(tok0))
        {
            if (world.exists)
            {
                world.do_update = 1;
            }
            if (do_record)
            {
                record(msg, 0);
            }
            char * tokplus = token_from_line(NULL);
            err_line(!(!tokplus), "Too many arguments, ignoring overflow.");
            free(msg_copy);
            return;
        }
    }
    err_line(1, "Unknown command/argument or bad number of tokens.");
    free(msg_copy);
}



extern uint8_t io_loop()
{
    while (1)
    {
        char * msg = io_round();
        server_test();
        if (!msg)
        {
            continue;
        }
        if (world.is_verbose)
        {
            exit_trouble(-1 == printf("Input: %s\n", msg), __func__, "printf");
        }
        uint8_t test = meta_commands(msg);
        if (test)
        {
            if (2 == test)
            {
                return 1;
            }
            continue;
        }
        if (world.replay)
        {
            free(msg);
            return 0;
        }
        obey_msg(msg, 1, 0);
        free(msg);
    }
}
