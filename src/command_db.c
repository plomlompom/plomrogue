/* command.c */

#include "command_db.h"
#include <stdlib.h> /* for free() */
#include <stdio.h> /* for FILE typedef, fgets() */
#include <stdint.h> /* for uint8_t */
#include <string.h> /* for strlen(), strtok() */
#include "main.h" /* for world global */
#include "rexit.h" /* for exit_err() */
#include "readwrite.h" /* for textfile_sizes(), try_fopen(), try_fclose() */
#include "misc.h" /* for try_malloc() */



/* Build string pointed to by "ch_ptr" from next token delimited by "delim". */
static void copy_tokenized_string(char ** ch_ptr, char * delim);



static void copy_tokenized_string(char ** ch_ptr, char * delim)
{
    char * f_name = "copy_tokenized_string()";
    char * dsc_ptr = strtok(NULL, delim);
    * ch_ptr = try_malloc(strlen(dsc_ptr) + 1, f_name);
    memcpy(* ch_ptr, dsc_ptr, strlen(dsc_ptr) + 1);
}



extern uint8_t is_command_id_shortdsc(uint8_t id, char * shortdsc)
{
    struct Command * cmd_ptr = world.cmd_db->cmds;
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



extern uint8_t get_command_id(char * dsc_short)
{
    struct Command * cmd_ptr = world.cmd_db->cmds;
    while (1)
    {
        if (0 == strcmp(dsc_short, cmd_ptr->dsc_short))
        {
            return cmd_ptr->id;
        }
        cmd_ptr = &cmd_ptr[1];
    }
}



extern char * get_command_longdsc(char * dsc_short)
{
    struct Command * cmd_ptr = world.cmd_db->cmds;
    while (1)
    {
        if (0 == strcmp(dsc_short, cmd_ptr->dsc_short))
        {
            return cmd_ptr->dsc_long;
        }
        cmd_ptr = &cmd_ptr[1];
    }
}



extern void init_command_db()
{
    char * f_name = "init_command_db()";
    char * err_s = "Trouble in init_cmds() with textfile_sizes().";

    char * path = "config/commands";
    FILE * file = try_fopen(path, "r", f_name);
    uint16_t lines, linemax;
    exit_err(textfile_sizes(file, &linemax, &lines), err_s);
    char line[linemax + 1];

    struct Command * cmds = try_malloc(lines * sizeof(struct Command), f_name);
    uint8_t i = 0;
    while (fgets(line, linemax + 1, file))
    {
        if ('\n' == line[0] || 0 == line[0])
        {
            break;
        }
        cmds[i].id = atoi(strtok(line, " "));
        copy_tokenized_string(&cmds[i].dsc_short, " ");
        copy_tokenized_string(&cmds[i].dsc_long, "\n");
        i++;
    }
    try_fclose(file, f_name);

    world.cmd_db = try_malloc(sizeof(struct CommandDB), f_name);
    world.cmd_db->cmds = cmds;
    world.cmd_db->n = lines;
}



extern void free_command_db()
{
    uint8_t i = 0;
    while (i < world.cmd_db->n)
    {
        free(world.cmd_db->cmds[i].dsc_short);
        free(world.cmd_db->cmds[i].dsc_long);
        i++;
    }
    free(world.cmd_db->cmds);
    free(world.cmd_db);
}
