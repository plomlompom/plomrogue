/* src/server/yx_uint8.h
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
