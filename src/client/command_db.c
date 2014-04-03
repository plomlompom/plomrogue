/* src/client/command_db.c */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "command_db.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* free() */
#include <string.h> /* memset(), strcmp(), strdup() */
#include "../common/err_try_fgets.h" /* err_line() */
#include "../common/parse_file.h" /* Context, EDIT_STARTED, parse_file(),
                                   * set_val()
                                   */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "array_append.h" /* array_append() */
#include "world.h" /* global world */
#include "cleanup.h" /* set_cleanup_flag() */



/* Flags for defining state of command DB config file entries. */
enum cmd_flag
{
    DESC_SET      = 0x02,
    SERVERCMD_SET = 0x04,
    SERVERARG_SET = 0x08,
    READY_CMD = DESC_SET
};



/* Get tokens from "context" and, by their order (in the individual context and
 * in subsequent calls of this function), interpret them as data to write into
 * the CommandDB.
 *
 * Individual CommandDB entries are put together line by line before being
 * written. Writing happens after all necessary members of an entry have been
 * assembled, and when additionally a) a new entry is started by a
 * context->token0 of "COMMAND"; or b) a NULL context->token0 is passed. This is
 * interpreted as the CommandDB read's end: appropriate cleanup flags are set.
 */
static void tokens_into_entries(struct Context * context);



static void tokens_into_entries(struct Context * context)
{
    char * f_name = "tokens_into_entries()";
    char * str_cmd = "COMMAND";
    static uint8_t cmd_flags = READY_CMD;
    static struct Command * cmd = NULL;
    if (!context->token0 || !strcmp(context->token0, str_cmd))
    {
        err_line((cmd_flags & READY_CMD) ^ READY_CMD, context->line,
                  context->err_pre, "Last definition block not finished yet.");
        if (cmd)
        {
            array_append(world.commandDB.n, sizeof(struct Command),
                         (void *) cmd, (void **) &world.commandDB.cmds);
            world.commandDB.n++;
            cmd = NULL;
        }
    }
    if (!context->token0)
    {
        set_cleanup_flag(CLEANUP_COMMANDS);
    }
    else if (!strcmp(context->token0, str_cmd))
    {
        cmd_flags = EDIT_STARTED;
        cmd = try_malloc(sizeof(struct Command), f_name);
        memset(cmd, 0, sizeof(struct Command));
        cmd->dsc_short = strdup(context->token1);
        err_line(NULL != get_command(cmd->dsc_short), context->line,
                 context->err_pre, "Declaration of ID already used.");
    }
    else if (!(   set_val(context, "DESCRIPTION", &cmd_flags,
                          DESC_SET, 's', (char *) &cmd->dsc_long)
               || set_val(context, "SERVER_COMMAND", &cmd_flags,
                          SERVERCMD_SET, 's', (char *) &cmd->server_msg)
               || set_val(context, "SERVER_ARGUMENT", &cmd_flags,
                          SERVERARG_SET, 'c', (char *) &cmd->arg)))
    {
        err_line(1, context->line, context->err_pre, "Unknown argument");
    }
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
    parse_file(world.path_commands, tokens_into_entries);
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
