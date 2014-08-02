/* src/server/god_commands.h
 *
 * God commands and their interpretation by the server.
 */



#ifndef GOD_COMMANDS_H
#define GOD_COMMANDS_H

#include <stdint.h> /* uint8_t */


/* Parse/apply god command "tok0" with argument "tok1", "tok2" etc. . */
extern uint8_t parse_god_command_1arg(char * tok0, char * tok1);
extern uint8_t parse_god_command_2arg(char * tok0, char * tok1, char * tok2);



#endif
