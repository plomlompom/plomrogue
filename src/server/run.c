/* src/server/run.c */

#include "run.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE, sprintf(), fflush() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), strcmp() strncmp(), atoi() */
#include <unistd.h> /* access() */
#include "../common/readwrite.h" /* try_fopen(), try_fcose(), try_fwrite(),
                                  * try_fgets(), try_fclose_unlink_rename(),
                                  * textfile_width(), try_fputc()
                                  */
#include "../common/rexit.h" /* exit_trouble(), exit_err() */
#include "ai.h" /* ai() */
#include "cleanup.h" /* unset_cleanup_flag() */
#include "init.h" /* remake_world() */
#include "io.h" /* io_round() */
#include "thing_actions.h" /* get_thing_action_id_by_name() */
#include "things.h" /* Thing, get_player() */
#include "world.h" /* global world */



/* Run the game world and its inhabitants (and their actions) until the player
 * avatar is free to receive new commands (or is dead).
 */
static void turn_over();

/* If "msg"'s first part matches "command_name", set player's Thing's .command
 * to the command's id and its .arg to a numerical value following in the latter
 * part of "msg" (if no digits are found, use 0); then finish player's turn and
 * turn game over to the NPCs via turn_over(); then return 1. Else, return 0.
 */
static uint8_t apply_player_command(char * msg, char * command_name);

/* Compares first line of file at world.path_out to world.server_test, aborts if
 * they don't match, but not before unsetting the flags deleting files in the
 * server directory, for in that case those must be assumed to belong to another
 * server process.
 */
static void server_test();



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
            struct ThingAction * ta = world.thing_actions;
            while (ta->id != thing->command)
            {
                ta = ta->next;
            }
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



static uint8_t apply_player_command(char * msg, char * command_name)
{
    if (!strncmp(msg, command_name, strlen(command_name)))
    {
        struct Thing * player = get_player();
        player->arg = atoi(&(msg[strlen(command_name)]));
        player->command = get_thing_action_id_by_name(command_name);
        turn_over();
        return 1;
    }
    return 0;
}



static void server_test()
{
    char * f_name = "server_test()";
    char test[10 + 1 + 10 + 1 + 1];
    FILE * file = try_fopen(world.path_out, "r", f_name);
    try_fgets(test, 10 + 10 + 1 + 1, file, f_name);
    try_fclose(file, f_name);
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



extern void obey_msg(char * msg, uint8_t do_record)
{
    char * f_name = "obey_msg()";
    if (   apply_player_command(msg, "wait")   /* TODO: Check for non-error   */
        || apply_player_command(msg, "move")   /* return value of a modified  */
        || apply_player_command(msg, "pick_up")/*get_thing_action_id_by_name()*/
        || apply_player_command(msg, "drop")   /* and if id found, execute on */
        || apply_player_command(msg, "use"));  /* it what's in                */
    else                                       /* apply_player_command().     */
    {
        char * seed_command = "seed";
        if (!strncmp(msg, seed_command, strlen(seed_command)))
        {
            remake_world(atoi(&(msg[strlen(seed_command)])));
        }
    }
    if (do_record)
    {
        char path_tmp[strlen(world.path_record) + strlen(world.tmp_suffix) + 1];
        sprintf(path_tmp, "%s%s", world.path_record, world.tmp_suffix);
        FILE * file_tmp  = try_fopen(path_tmp, "w", f_name);
        if (!access(world.path_record, F_OK))
        {
            FILE * file_read = try_fopen(world.path_record, "r", f_name);
            uint32_t linemax = textfile_width(file_read);
            char line[linemax + 1];
            while (try_fgets(line, linemax + 1, file_read, f_name))
            {
                try_fwrite(line, strlen(line), 1, file_tmp, f_name);
            }
            try_fclose(file_read, f_name);
        }
        try_fwrite(msg, strlen(msg), 1, file_tmp, f_name);
        try_fputc('\n', file_tmp, f_name);
        try_fclose_unlink_rename(file_tmp, path_tmp, world.path_record, f_name);
    }
}



extern uint8_t io_loop()
{
    char * f_name = "io_loop()";
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
            exit_trouble(-1 == printf("Input: %s\n", msg), f_name, "printf()");
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
            try_fwrite(pong, strlen(pong), 1, world.file_out, f_name);
            fflush(world.file_out);
            continue;
        }
        if (world.replay)
        {
            free(msg);
            return 0;
        }
        obey_msg(msg, 1);
        free(msg);
    }
}
