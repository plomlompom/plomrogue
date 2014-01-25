/* src/client/command_db.c */

#include "command_db.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* FILE */
#include <stdlib.h> /* free() */
#include <string.h> /* memcpy(), strlen(), strtok(), strcmp() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), try_fgets()
                                  * textfile_sizes()
                                  */
#include "../common/rexit.h" /* for exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "world.h" /* global world */



/* Point "ch_ptr" to next strtok() string in "line" delimited by "delim".*/
static void copy_tokenized_string(char * line, char ** ch_ptr, char * delim);



static void copy_tokenized_string(char * line, char ** ch_ptr, char * delim)
{
    char * f_name = "copy_tokenized_string()";
    char * dsc_ptr = strtok(line, delim);
    * ch_ptr = try_malloc(strlen(dsc_ptr) + 1, f_name);
    memcpy(* ch_ptr, dsc_ptr, strlen(dsc_ptr) + 1);
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
        struct Command cmd;
        copy_tokenized_string(line, &cmd.dsc_short, delim);
        copy_tokenized_string(NULL, &cmd.server_msg, delim);
        if (!strcmp("0", cmd.server_msg))
        {                          /* A .server_msg == NULL helps control.c's */
            free(cmd.server_msg);  /* try_key() and try_server_command() to   */
            cmd.server_msg = NULL; /* differentiate server commands from      */
        }                          /* non-server commands.                    */
        char * arg_string = strtok(NULL, delim);
        cmd.arg = arg_string[0];
        copy_tokenized_string(NULL, &cmd.dsc_long, "\n");
        uint32_t old_size = i * sizeof(struct Command);
        uint32_t new_size = old_size + sizeof(struct Command);
        struct Command * new_cmds = try_malloc(new_size, f_name);
        memcpy(new_cmds, world.commandDB.cmds, old_size);
        new_cmds[i] = cmd;
        free(world.commandDB.cmds);
        world.commandDB.cmds = new_cmds;
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
