/* src/client/command_db.c */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "command_db.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* free() */
#include <string.h> /* strcmp(), strdup() */
#include "../common/parse_file.h" /* EDIT_STARTED,parse_init_entry(),
                                   * parse_id_uniq(), parse_unknown_arg(),
                                   * parsetest_too_many_values(), parse_file(),
                                   * parse_and_reduce_to_readyflag(),
                                   * parse_flagval()
                                   */
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



/* Interpret "token0" and "token1" as data to write into the CommandDB.
 *
 * Individual CommandDB entries are put together line by line before being
 * written. Writing happens after all necessary members of an entry have been
 * assembled, and when additionally a) a new entry is started by a "token0" of
 * "COMMAND"; or b) of "token0" of NULL is passed.
 */
static void tokens_into_entries(char * token0, char * token1);



static void tokens_into_entries(char * token0, char * token1)
{
    char * str_cmd = "COMMAND";
    static uint8_t cmd_flags = READY_CMD;
    static struct Command * cmd = NULL;
    if (!token0 || !strcmp(token0, str_cmd))
    {
        parse_and_reduce_to_readyflag(&cmd_flags, READY_CMD);
        if (cmd)
        {
            array_append(world.commandDB.n, sizeof(struct Command),
                         (void *) cmd, (void **) &world.commandDB.cmds);
            world.commandDB.n++;
            free(cmd);
            cmd = NULL;
        }
    }
    if (token0)
    {
        parsetest_too_many_values();
        if      (!strcmp(token0, str_cmd))
        {
            cmd = (struct Command *) parse_init_entry(&cmd_flags,
                                                      sizeof(struct Command));
            cmd->dsc_short = strdup(token1);
            parse_id_uniq(NULL != get_command(cmd->dsc_short));
        }
        else if (!(   parse_flagval(token0, token1, "DESCRIPTION", &cmd_flags,
                                    DESC_SET, 's', (char *) &cmd->dsc_long)
                   || parse_flagval(token0, token1,"SERVER_COMMAND", &cmd_flags,
                                    SERVERCMD_SET, 's',(char *)&cmd->server_msg)
                   || parse_flagval(token0, token1,"SERVER_ARGUMENT",&cmd_flags,
                                    SERVERARG_SET, 'c', (char *) &cmd->arg)))
        {
            parse_unknown_arg();
        }
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
