/* src/server/rrand.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
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
