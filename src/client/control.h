/* src/client/control.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Routines for handling control input from the keyboard.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h> /* uint8_t, uint16_t */



/* Try to match "key" to the Command DB and execute appropriate actions if
 * successful. Return 0 on failure, 2 for calling a "quit" comand (to be
 * handled externally) and 1 for calling any other command.
 */
extern uint8_t try_key(uint16_t key);



#endif
