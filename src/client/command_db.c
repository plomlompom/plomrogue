/* src/client/command_db.c */

#include "command_db.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* memset(), strlen(), strcmp() */
#include "../common/err_try_fgets.h" /* reset_err_try_fgets_counter() */
#include "../common/readwrite.h" /* try_fopen(),try_fclose(),textfile_width() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "misc.h" /* array_append() */
#include "world.h" /* global world */
#include "cleanup.h" /* set_cleanup_flag() */



/* Helper to init_command_db(). */
static void write_line_to_target(char ** target, char * line)
{
    char * f_name = "write_line_to_target()";
    *target = try_malloc(strlen(line), f_name);
    line[strlen(line) - 1] = '\0';
    sprintf(*target, "%s", line);
}



extern struct Command * get_command(char * dsc_short)
{
    struct Command * cmd_ptr = world.commandDB.cmds;
    uint8_t i = 0;
    while (i < world.commandDB.n)
    {
        if (0 == strcmp(dsc_short, cmd_ptr->dsc_short))
        {
            return cmd_ptr;
        }
        cmd_ptr = &cmd_ptr[1];
        i++;
    }
    return NULL;
}



extern void init_command_db()
{
    char * f_name = "init_command_db()";
    char * context = "Failed reading command DB file. ";
    FILE * file = try_fopen(world.path_commands, "r", f_name);
    uint32_t linemax = textfile_width(file);
    char line[linemax + 1];
    reset_err_try_fgets_counter();
    uint8_t i = 0;
    while (1)
    {
        int test_for_end = try_fgetc(file, f_name);
        if (EOF == test_for_end || '\n' == test_for_end)
        {
            break;
        }
        exit_trouble(EOF == ungetc(test_for_end, file), f_name, "ungetc()");
        struct Command cmd;
        memset(&cmd, 0, sizeof(struct Command));
        err_try_fgets(line, linemax, file, context, "nf");
        write_line_to_target(&cmd.dsc_short, line);
        err_try_fgets(line, linemax, file, context, "0nf");
        write_line_to_target(&cmd.dsc_long, line);
        err_try_fgets(line, linemax, file, context, "0nf");
        if (strcmp(world.delim, line))
        {
            write_line_to_target(&cmd.server_msg, line);
            err_try_fgets(line, linemax, file, context, "0nfs");
            cmd.arg = line[0];
            err_try_fgets(line, linemax, file, context, "d");
        }
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
