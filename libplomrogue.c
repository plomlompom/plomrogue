#include <stdint.h> /* uint16_t, uint32_t */



/* Pseudo-randomness seed for rrand(), set by seed_rrand(). */
static uint32_t seed = 0;



/* With set_seed set, set seed global to seed_input. In any case, return it. */
extern uint32_t seed_rrand(uint8_t set_seed, uint32_t seed_input)
{
    if (set_seed)
    {
        seed = seed_input;
    }
    return seed;
}



/* Return 16-bit number pseudo-randomly generated via Linear Congruential
 * Generator algorithm with some proven constants. Use instead of any rand() to
  * ensure portability of the same pseudo-randomness across systems.
 */
extern uint16_t rrand()
{   /* Constants as recommended by POSIX.1-2001 (see man page rand(3)). */
    seed = ((seed * 1103515245) + 12345) % 4294967296;
    return (seed >> 16); /* Ignore less random least significant bits. */
}
