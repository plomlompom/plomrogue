/* src/server/rrand.h
 *
 * Provides deterministic pseudo-randomness.
 */

#ifndef RRAND_H
#define RRAND_H

#include <stdint.h> /* uint16_t */



/* Return 16-bit number pseudo-randomly generated via Linear Congruential
 * Generator algorithm with some proven constants. Use instead of rand() to
 * ensure portability of the same pseudo-randomness across systems.
 */
extern uint16_t rrand();



#endif
