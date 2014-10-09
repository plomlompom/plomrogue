/* src/server/yx_uint8.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Struct used for game map representations.
 */

#ifndef YX_UINT8_H
#define YX_UINT8_H

#include <stdint.h> /* uint8_t */



/* Coordinate for maps of max. 256x256 cells. */
struct yx_uint8
{
    uint8_t y;
    uint8_t x;
};



#endif
