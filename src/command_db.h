/* command.h
 *
 * The Command DB collects all commands the user can give. It only contains
 * identifiers and descriptions of commands, not the functions executing these
 * commands. Coupling with those happens elsewhere.
 */

#ifndef COMMAND_DB_H
#define COMMAND_DB_H



#include <stdint.h> /* for uint8_t */
struct World;



struct Command
{
    uint8_t id;       /* unique identifier of command */
    char * dsc_short; /* short string name of command to be used internally */
    char * dsc_long;  /* long string description of command for the  user */
};

struct CommandDB
{
    uint8_t n;             /* number of Command structs in database*/
    struct Command * cmds; /* pointer to first Command struct in database */
};



/* Give short description of command ("dsc_short"), get long descrption. */
extern char * get_command_longdsc(struct World * world, char * dsc_short);



/* Read in CommandDB from file "config/commands" to world.cmd_db. */
extern void init_command_db(struct World * world);



/* Free all memory allocated with init_command_db. */
extern void free_command_db(struct World * world);



#endif
