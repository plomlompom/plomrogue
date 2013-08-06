/* rrand.h
 *
 * Provide pseudo-random numbers via a Linear Congruential Generator algorithm
 * with some proven constants. Use these functions instead of rand() and
 * srand() to ensure portable pseudo-randomness portability.
 */



#ifndef RRAND_H
#define RRAND_H

#include <stdint.h>    /* for uint32_t */



/* Return 16-bit number pseudo-randomly generated. */
extern uint16_t rrand();



/* Set seed that rrand() starts from. */
extern void rrand_seed(uint32_t new_seed);



#endif
