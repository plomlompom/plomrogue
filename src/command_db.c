/* command.c */

#include "command_db.h"
#include <stdlib.h> /* for malloc() */
#include <stdio.h> /* for FILE typedef, fopen(), fclose(), fgets() */
#include <stdint.h> /* for uint8_t */
#include <string.h> /* for strlen(), strtok() */
#include "main.h" /* for World struct */
#include "rexit.h" /* for exit_err() */
#include "misc.h" /* for textfile_sizes() */



/* Build string pointed to by "ch_ptr" from next token delimited by "delim",
 * exit_err() with "err" as error message on malloc() failure.
 */
static void copy_tokenized_string(struct World * world,
                                 char ** ch_ptr, char * delim, char * err)
{
    char * dsc_ptr = strtok(NULL, delim);
    * ch_ptr = malloc(strlen(dsc_ptr) + 1);
    exit_err(NULL == * ch_ptr, world, err);
    memcpy(* ch_ptr, dsc_ptr, strlen(dsc_ptr) + 1);
}



extern uint8_t is_command_id_shortdsc(struct World * world,
                                      uint8_t id, char * shortdsc)
{
    struct Command * cmd_ptr = world->cmd_db->cmds;
    while (1)
    {
        if (id == cmd_ptr->id)
        {
            if (strcmp(shortdsc, cmd_ptr->dsc_short))
            {
                return 0;
            }
            return 1;
        }
        cmd_ptr = &cmd_ptr[1];
    }
}



extern uint8_t get_command_id(struct World * world, char * dsc_short)
{
    struct Command * cmd_ptr = world->cmd_db->cmds;
    while (1)
    {
        if (0 == strcmp(dsc_short, cmd_ptr->dsc_short))
        {
            return cmd_ptr->id;
        }
        cmd_ptr = &cmd_ptr[1];
    }
}



extern char * get_command_longdsc(struct World * world, char * dsc_short)
{
    struct Command * cmd_ptr = world->cmd_db->cmds;
    while (1)
    {
        if (0 == strcmp(dsc_short, cmd_ptr->dsc_short))
        {
            return cmd_ptr->dsc_long;
        }
        cmd_ptr = &cmd_ptr[1];
    }
}



extern void init_command_db(struct World * world)
{
    char * err = "Trouble in init_cmds() with fopen() on file 'commands'.";
    FILE * file = fopen("config/commands", "r");
    exit_err(NULL == file, world, err);
    uint16_t lines, linemax;
    textfile_sizes(file, &linemax, &lines);
    err = "Trouble in init_cmds() with malloc().";
    char * line = malloc(linemax);
    exit_err(NULL == line, world, err);
    struct Command * cmds = malloc(lines * sizeof(struct Command));
    exit_err(NULL == line, world, err);
    uint8_t i = 0;
    while (fgets(line, linemax, file))
    {
        cmds[i].id = atoi(strtok(line, " "));
        copy_tokenized_string(world, &cmds[i].dsc_short, " ", err);
        copy_tokenized_string(world, &cmds[i].dsc_long, "\n", err);
        i++;
    }
    free(line);
    world->cmd_db = malloc(sizeof(struct CommandDB));
    world->cmd_db->cmds = cmds;
    world->cmd_db->n = lines;
    err = "Trouble in init_cmds() with fclose() on file 'commands'.";
    exit_err(fclose(file), world, err);
}



extern void free_command_db(struct World * world)
{
    uint8_t i = 0;
    while (i < world->cmd_db->n)
    {
        free(world->cmd_db->cmds[i].dsc_short);
        free(world->cmd_db->cmds[i].dsc_long);
        i++;
    }
    free(world->cmd_db->cmds);
    free(world->cmd_db);
}
