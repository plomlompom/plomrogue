/* src/client/control.h
 *
 * Routines for handling control input from the keyboard.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h> /* uint8_t, uint16_t */



/* Try to call by "key" player actions triggering messages to the server. Return
 * 1 on success, 0 on failure.
 */
extern uint8_t player_control(int key);

/* Try to call by "key" actions for active window's keybindings/geometry config
 * view. Return 1 on success, 0 on failure.
 */
extern uint8_t wingeom_control(int key);
extern uint8_t winkeyb_control(int key);

/* Try to call by "key" basic window and game management actions that do not
 * send a message to the server and do not change individual windows'
 * keybindings or geometries. Returns 0 on failure, 2 for calling a "quit"
 * action (to be handled externally) and 1 for calling any other action.
 */
extern uint8_t meta_control(int key);



#endif
