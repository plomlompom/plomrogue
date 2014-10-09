/* src/server/rrand.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "rrand.h"
#include <stdint.h> /* uint16_t */
#include "world.h" /* global world */



extern uint16_t rrand()
{   /* Constants as recommended by POSIX.1-2001 (see man page rand(3)). */
    world.seed = ((world.seed * 1103515245) + 12345) % 4294967296;
    return (world.seed >> 16); /* Ignore less random least significant bits. */
}
