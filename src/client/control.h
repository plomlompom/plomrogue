/* src/client/control.h
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
