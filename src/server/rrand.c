/* src/server/rrand.c */

#include "rrand.h"
#include <stdint.h> /* uint16_t */
#include "world.h" /* global world */



extern uint16_t rrand()
{   /* Constants as recommended by POSIX.1-2001 (see man page rand(3)). */
    world.seed = ((world.seed * 1103515245) + 12345) % 4294967296;
    return (world.seed >> 16); /* Ignore less random least significant bits. */
}
