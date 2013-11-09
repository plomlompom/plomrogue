/* command.h
 *
 * The Command DB collects all commands the user can give. It only contains
 * identifiers and descriptions of commands, not the functions executing these
 * commands. Coupling with those happens elsewhere.
 */

#ifndef COMMAND_DB_H
#define COMMAND_DB_H

#include <stdint.h> /* for uint8_t */



struct Command
{
    uint8_t id;       /* unique identifier of command */
    char * dsc_short; /* short string name of command to be used internally */
    char * dsc_long;  /* long string description of command for the user */
};

struct CommandDB
{
    uint8_t n;             /* number of Command structs in database*/
    struct Command * cmds; /* pointer to first Command struct in database */
};



/* Is "id" the ID of command whose dsc_short is "shortdsc"? Answer in binary. */
extern uint8_t is_command_id_shortdsc(uint8_t id, char * shortdsc);

/* Give short description of command ("dsc_short"), get its ID. */
extern uint8_t get_command_id(char * dsc_short);

/* Give short description of command ("dsc_short"), get long description. */
extern char * get_command_longdsc(char * dsc_short);

/* Read in CommandDB from file "config/commands" to world.cmd_db. */
extern void init_command_db();

/* Free all memory allocated with init_command_db. */
extern void free_command_db();



#endif
