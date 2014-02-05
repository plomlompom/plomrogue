/* yx_uint16.h
 *
 * Used for window / screen maps and game map size.
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
