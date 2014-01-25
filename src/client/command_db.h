/* src/client/command_db.h
 *
 * The Command DB collects the commands available to the user by internal name,
 * description and, optionally, components of a message to send to the server
 * when calling it.
 */

#ifndef COMMAND_DB_H
#define COMMAND_DB_H

#include <stdint.h> /* uint8_t */



struct Command
{
    char * dsc_short; /* short name of command to be used internally */
    char * dsc_long; /* long description of command to be shown to the user */
    char * server_msg; /* optionally start string of message to send to server*/
    char arg; /* defines server message suffix by player_control() convention */
};

struct CommandDB
{
    struct Command * cmds; /* memory area for sequence of all Command structs */
    uint8_t n;             /* number of Command structs in database */
};



/* Return Command struct for command described by its "dsc_short" member. */
extern struct Command * get_command(char * dsc_short);

/* Reads in CommandDB config file line by line until end or first empty line. */
extern void init_command_db();

/* Free all memory allocated with init_command_db. */
extern void free_command_db();



#endif
