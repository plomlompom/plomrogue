/* src/server/run.c */

#include "run.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), strncmp(), atoi() */
#include <unistd.h> /* access() */
#include "../common/readwrite.h" /* try_fopen(), try_fcose(), try_fwrite(),
                                  * try_fgets(), try_fclose_unlink_rename(),
                                  * textfile_sizes(), try_fputc()
                                  */
#include "../common/rexit.h" /* exit_trouble() */
#include "ai.h" /* pretty_dumb_ai() */
#include "init.h" /* remake_world() */
#include "io.h" /* io_round() */
#include "map_object_actions.h" /* get_moa_id_by_name() */
#include "map_objects.h" /* struct MapObj, get_player() */
#include "world.h" /* global world */



/* Run the game world and its inhabitants (and their actions) until the player
 * avatar is free to receive new commands (or is dead).
 */
static void turn_over();

/* If "msg"'s first part matches "command_name", set player's MapObj's .command
 * to the command's id and its .arg to a numerical value following in the latter
 * part of "msg" (if no digits are found, use 0); then finish player's turn and
 * turn game over to the NPCs via turn_over(); then return 1. Else, return 0.
 */
static uint8_t apply_player_command(char * msg, char * command_name);



static void turn_over()
{
    struct MapObj * player = get_player();
    struct MapObj * map_object = player;
    uint16_t start_turn = world.turn;
    uint8_t first_round = 1;
    while (    0 < player->lifepoints
           || (0 == player->lifepoints && start_turn == world.turn))
    {
        if (NULL == map_object)
        {
            world.turn++;
            map_object = world.map_objs;
        }
        if (0 < map_object->lifepoints)
        {
            if (0 == first_round && 0 == map_object->progress)
            {
                if (map_object == player)
                {
                    break;
                }
                pretty_dumb_ai(map_object);
            }
            first_round = 0;
            map_object->progress++;
            struct MapObjAct * moa = world.map_obj_acts;
            while (moa->id != map_object->command)
            {
                moa = moa->next;
            }
            if (map_object->progress == moa->effort)
            {
                moa->func(map_object);
                map_object->progress = 0;
            }
        }
        map_object = map_object->next;
    }
}



static uint8_t apply_player_command(char * msg, char * command_name)
{
    if (!strncmp(msg, command_name, strlen(command_name)))
    {
        struct MapObj * player = get_player();
        player->arg = atoi(&(msg[strlen(command_name)]));
        player->command = get_moa_id_by_name(command_name);
        turn_over();
        return 1;
    }
    return 0;
}



extern void obey_msg(char * msg, uint8_t do_record)
{
    char * f_name = "obey_msg()";
    if (   apply_player_command(msg, "wait")   /* TODO: Check for non-error   */
        || apply_player_command(msg, "move")   /* return value of a modified  */
        || apply_player_command(msg, "pick_up")/* get_moa_id_by_name(); if id */
        || apply_player_command(msg, "drop")   /* found, execute on it what's */
        || apply_player_command(msg, "use"));  /* in apply_player_command().  */
    else
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
            uint32_t linemax = textfile_sizes(file_read, NULL);
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
        if (world.replay)
        {
            free(msg);
            return 0;
        }
        obey_msg(msg, 1);
        free(msg);
    }
}