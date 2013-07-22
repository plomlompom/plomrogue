/*  yx_uint16.h
 *
 *  Structs and routines for coordinates and movement in 2-dimensional space
 *  (such as the ncurses screen and game maps).
 */

#ifndef YX_UINT16_H
#define YX_UINT16_H

#include <stdint.h>

/* Coordinates for maps of max. 65536x65536 cells. */
struct yx_uint16
{
    uint16_t y;
    uint16_t x;
};

/* This encodes directions. */

enum dir
{
    NORTH = 1,
    EAST  = 2,
    SOUTH = 3,
    WEST  = 4
};

/* Return 1 if two yx_uint16 coordinates a and b are equal, else 0. */
extern char yx_uint16_cmp(struct yx_uint16 a, struct yx_uint16 b);

/* Return yx_uint16 coordinate one step from coordinate yx in direction dir. */
extern struct yx_uint16 mv_yx_in_dir(enum dir d, struct yx_uint16 yx);

#endif
