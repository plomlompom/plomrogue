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
    world.commandDB.cmds = try_malloc(lines * sizeof(struct Command), f_name);
    uint8_t i = 0;
    char * delim = " ";
    while (try_fgets(line, linemax + 1, file, f_name))
    {
        if ('\n' == line[0] || 0 == line[0])
        {
            break;
        }
        copy_tokenized_string(line, &world.commandDB.cmds[i].dsc_short, delim);
        copy_tokenized_string(NULL, &world.commandDB.cmds[i].server_msg, delim);
        if (!strcmp("0", world.commandDB.cmds[i].server_msg))
        {                                             /*.server_msg==0 detects*/
            free(world.commandDB.cmds[i].server_msg); /* non-server commands  */
            world.commandDB.cmds[i].server_msg = NULL;/* in try_key() /       */
        }                                             /* try_server_command().*/
        char * arg_string = strtok(NULL, delim);
        world.commandDB.cmds[i].arg = arg_string[0];
        copy_tokenized_string(NULL, &world.commandDB.cmds[i].dsc_long, "\n");
        i++;
    }
    try_fclose(file, f_name);
    world.commandDB.n = lines;
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
