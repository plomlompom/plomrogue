/* src/server/god_commands.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * God commands and their interpretation by the server.
 */

#ifndef GOD_COMMANDS_H
#define GOD_COMMANDS_H

#include <stdint.h> /* uint8_t */



/* Parse/apply god command "tok0" with argument "tok1", "tok2" etc. . */
extern uint8_t parse_god_command_1arg(char * tok0, char * tok1);
extern uint8_t parse_god_command_2arg(char * tok0, char * tok1, char * tok2);
extern uint8_t parse_god_command_3arg(char * tok0, char * tok1, char * tok2,
                                      char * tok3);



#endif
