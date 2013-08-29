#include "rrand.h"
#include <stdint.h> /* for uint16_t, uint32_t */



static uint32_t seed = 0;



extern uint16_t rrand()
{
    /* Constants as recommended by POSIX.1-2001 (see man page rand(3)). */
    seed = ((seed * 1103515245) + 12345) % 2147483647;

    return (seed >> 16);     /* Ignore less random least significant 16 bits. */
}



extern void rrand_seed(uint32_t new_seed)
{
    seed = new_seed;
}
