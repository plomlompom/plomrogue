/* src/server/run.c */

#define _POSIX_C_SOURCE 200809L
#include "run.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE, printf(), fflush() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), strcmp(), strncmp(), strdup() */
#include <unistd.h> /* access() */
#include "../common/parse_file.h" /* set_err_line_options(), token_from_line(),
                                   * err_line(), err_line_inc(), parse_val()
                                   */
#include "../common/readwrite.h" /* try_fopen(), try_fcose(), try_fwrite(),
                                  * try_fgets(), textfile_width(), try_fputc()
                                  */
#include "../common/rexit.h" /* exit_trouble(), exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "ai.h" /* ai() */
#include "cleanup.h" /* unset_cleanup_flag() */
#include "god_commands.h" /* parse_god_command_1arg() */
#include "hardcoded_strings.h" /* s */
#include "io.h" /* io_round(), save_world() */
#include "things.h" /* Thing, get_thing_action_id_by_name(), get_player() */
#include "world.h" /* world */



/* If "string" and "comparand" match in string, set "c_to_set" to value."  */
static uint8_t set_char_by_string_comparison(char * string, char * comparand,
                                             char * c_to_set, char value);

/* Return 1 on world.exists, else 0 and err_line() appropriate error message. */
static uint8_t player_commands_allowed();

/* Parse player command "tok0" with no argument to player action, comment on
 * invalidity of non-zero "tok1" (but do not abort in that case).
 */
static uint8_t parse_player_command_0arg(char * tok0, char * tok1);

/* Parse player command "tok0" with one argument "tok1" to player action. */
static uint8_t parse_player_command_1arg(char * tok0, char * tok1);

/* Parse/apply command "tok0" with argument "tok1" and test the line for further
 * tokens, commenting on their invalidity (but don't abort on finding them).
 */
static uint8_t parse_command_1arg(char * tok0, char * tok1);


/* Compares first line of server out file to world.server_test, aborts if they
 * don't match, but not before unsetting the flags deleting files in the server
 * directory, for in that case those must be assumed to belong to another server
 * process.
 */
static void server_test();

/* Run the game world and its inhabitants (and their actions) until the player
 * avatar is free to receive new commands (or is dead).
 */
static void turn_over();




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
        err_line(1, "No world exists in which to run player commands.");
        return 0;
    }
    return 1;
}



static uint8_t parse_player_command_0arg(char * tok0, char * tok1)
{
    struct Thing * player = get_player();
    if (!strcmp(tok0, s[S_CMD_WAIT]) || !strcmp(tok0, s[S_CMD_PICKUP]))
    {
        if (player_commands_allowed())
        {
            player->command = get_thing_action_id_by_name(tok0);
            player->arg = 0;
            turn_over();
            err_line (NULL != tok1, "No arguments expected, ignoring them.");
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



static uint8_t parse_command_1arg(char * tok0, char * tok1)
{
    char * tok2 = token_from_line(NULL);
    if (   parse_player_command_1arg(tok0, tok1)
        || parse_god_command_1arg(tok0, tok1));
    else
    {
        return 0;
    }
    char * err = "But one argument expected, ignoring further arguments.";
    err_line (NULL != tok2, err);
    return 1;
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



static void turn_over()
{
    struct Thing * player = get_player();
    struct Thing * thing = player;
    uint16_t start_turn = world.turn;
    while (    0 < player->lifepoints
           || (0 == player->lifepoints && start_turn == world.turn))
    {
        if (NULL == thing)
        {
            world.turn++;
            thing = world.things;
        }
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
        thing = thing->next;
    }
}



static void record_msg(char * msg)
{
    char * path_tmp;
    FILE * file_tmp = atomic_write_start(s[S_PATH_RECORD], &path_tmp);
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
    try_fwrite(msg, strlen(msg), 1, file_tmp, __func__);
    try_fputc('\n', file_tmp, __func__);
    atomic_write_finish(file_tmp, s[S_PATH_RECORD], path_tmp);
}



extern void obey_msg(char * msg, uint8_t do_record, uint8_t do_verbose)
{
    if (world.is_verbose && do_verbose)
    {
        exit_trouble(-1 == printf("Input: %s\n", msg), __func__, "printf");
    }
    set_err_line_options("Trouble with message: ", msg, 0);
    char * msg_copy = strdup(msg);
    if (msg[0] == 'm')
    {
        int a = 5;
        a = a;
    }
    char * tok0 = token_from_line(msg_copy);
    if (NULL != tok0)
    {
        char * tok1 = token_from_line(NULL);
        if (    parse_player_command_0arg(tok0, tok1)
            || (tok1 && parse_command_1arg(tok0, tok1)))
        {
            if (world.exists)
            {
                world.do_update = 1;
            }
            if (do_record)
            {
                save_world();
                record_msg(msg);
            }
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
        server_test();
        char * msg = io_round();
        if (NULL == msg)
        {
            continue;
        }
        if (world.is_verbose)
        {
            exit_trouble(-1 == printf("Input: %s\n", msg), __func__, "printf");
        }
        if (!strcmp("QUIT", msg))
        {
            free(msg);
            return 1;
        }
        if (!strcmp("PING", msg))
        {
            free(msg);
            char * pong = "PONG\n";
            try_fwrite(pong, strlen(pong), 1, world.file_out, __func__);
            fflush(world.file_out);
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
