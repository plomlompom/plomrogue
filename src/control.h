/* control.h
 */

#ifndef CONTROL_H
#define CONTROL_H



#include <stdint.h>    /* for uint8_t */
#include "yx_uint16.h" /* for yx_uint16 coordinates */
struct World;



/* Call some meta game / window management actions dependent on key. If the
 * "quit" action is called, return 1 only instead of doing anything directly.
 */
extern uint8_t meta_keys(int key, struct World * world);



#endif
