/* src/client/command_db.c */

#include "command_db.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* memcpy(), strlen(), strtok(), strcmp() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), try_fgets()
                                  * textfile_sizes()
                                  */
#include "../common/rexit.h" /* for exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "misc.h" /* array_append() */
#include "world.h" /* global world */



/* Helpers to init_command_db(). */
static uint8_t copy_tokenized_string(char * line, char ** ch_ptr, char * delim);
static char * init_command_db_err(char * line_copy, uint8_t line_number);


static uint8_t copy_tokenized_string(char * line, char ** ch_ptr, char * delim)
{
    char * f_name = "copy_tokenized_string()";
    char * dsc_ptr = strtok(line, delim);
    if (!dsc_ptr)
    {
        return 1;
    }
    * ch_ptr = try_malloc(strlen(dsc_ptr) + 1, f_name);
    memcpy(* ch_ptr, dsc_ptr, strlen(dsc_ptr) + 1);
    return 0;
}



static char * init_command_db_err(char * line_copy, uint8_t line_number)
{
    char * f_name = "init_command_db_err";
    char * err_start = "Failed reading command config file at ";
    char * err_middle = " due to malformed line ";
    line_copy[strlen(line_copy) - 1] = '\0';
    char * err = try_malloc(strlen(err_start) + strlen(world.path_commands) +
                            strlen(err_middle) + 3 + 2 + strlen(line_copy) + 1,
                            f_name);
    sprintf(err, "%s%s%s%d: %s", err_start, world.path_commands, err_middle,
            line_number, line_copy);
    return err;
}



extern struct Command * get_command(char * dsc_short)
{
    struct Command * cmd_ptr = world.commandDB.cmds;
    uint8_t i = 0;
    while (i < world.commandDB.n)
    {
        if (0 == strcmp(dsc_short, cmd_ptr->dsc_short))
        {
            break;
        }
        cmd_ptr = &cmd_ptr[1];
        i++;
    }
    char * err_start = "get_command_data() failed on request for: ";
    char err[strlen(err_start) + strlen(dsc_short) + 1];
    sprintf(err, "%s%s", err_start, dsc_short);
    exit_err(i == world.commandDB.n, err);
    return cmd_ptr;
}



extern void init_command_db()
{
    char * f_name = "init_command_db()";
    FILE * file = try_fopen(world.path_commands, "r", f_name);
    uint32_t lines;
    uint32_t linemax = textfile_sizes(file, &lines);
    char line[linemax + 1];
    uint8_t i = 0;
    char * delim = " ";
    while (try_fgets(line, linemax + 1, file, f_name))
    {
        if ('\n' == line[0] || 0 == line[0])
        {
            break;
        }
        char line_copy[strlen(line) + 1];
        sprintf(line_copy, "%s", line);
        struct Command cmd;
        char * arg_string;
        exit_err((   copy_tokenized_string(line, &cmd.dsc_short, delim)
                 || copy_tokenized_string(NULL, &cmd.server_msg, delim)
                 || NULL == (arg_string = strtok(NULL, delim))
                 || copy_tokenized_string(NULL, &cmd.dsc_long, "\n")),
                 init_command_db_err(line_copy, i + 1));
        cmd.arg = arg_string[0];
        if (!strcmp("0", cmd.server_msg))
        {                          /* A .server_msg == NULL helps control.c's */
            free(cmd.server_msg);  /* try_key() and try_server_command() to   */
            cmd.server_msg = NULL; /* differentiate server commands from      */
        }                          /* non-server commands.                    */
        array_append(i, sizeof(struct Command), (void *) &cmd,
                     (void **) &world.commandDB.cmds);
        i++;
    }
    try_fclose(file, f_name);
    world.commandDB.n = i;
    set_cleanup_flag(CLEANUP_COMMANDS);
}



extern void free_command_db()
{
    uint8_t i = 0;
    while (i < world.commandDB.n)
    {
        free(world.commandDB.cmds[i].dsc_short);
        free(world.commandDB.cmds[i].dsc_long);
        free(world.commandDB.cmds[i].server_msg);
        i++;
    }
    free(world.commandDB.cmds);
}
