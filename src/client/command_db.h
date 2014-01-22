/* src/client/command_db.h
 *
 * The Command DB collects identifiers and verbal descriptions of all commands
 * the user can give.
 */

#ifndef COMMAND_DB_H
#define COMMAND_DB_H

#include <stdint.h> /* uint8_t */



struct Command
{
    char * dsc_short; /* short string name of command to be used internally */
    char * dsc_long;  /* long string description of command for the user */
};

struct CommandDB
{
    struct Command * cmds; /* memory area for sequence of all Command structs */
    uint8_t n;             /* number of Command structs in database */
};



/* Give short description of command ("dsc_short"), get long description. */
extern char * get_command_longdsc(char * dsc_short);

/* Reads CommandDB from CommandDB file, line by line, until first empty line. */
extern void init_command_db();

/* Free all memory allocated with init_command_db. */
extern void free_command_db();



#endif
