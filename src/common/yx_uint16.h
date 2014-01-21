/* yx_uint16.h
 *
 * Struct coordinates in 2-dimensional space (such as the ncurses screen and
 * game maps).
 */

#ifndef YX_UINT16_H
#define YX_UINT16_H

#include <stdint.h> /* for uint16_t */



/* Coordinates for maps of max. 65536x65536 cells. */
struct yx_uint16
{
    uint16_t y;
    uint16_t x;
};



#endif
