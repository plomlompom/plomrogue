/* control.h
 *
 * Routines for handling control input from keyboard or record file.
 */

#ifndef CONTROL_H
#define CONTROL_H



#include <stdint.h> /* for uint8_t */
struct World;



/* Control the player character, either via recorded "action" or pressed "key".
 */
extern void record_control(int action, struct World * world);
extern uint8_t player_control(int key, struct World * world);



/* Call by "key" game / window management actions that don't influence the
 * player character. If the "quit" action is called, return 1 (instead of
 * exiting directly).
 */
extern uint8_t meta_control(int key, struct World * world);



#endif
