/* yx_uint16.h
 *
 * Structs and routines for coordinates and movement in 2-dimensional space
 * (such as the ncurses screen and game maps).
 */

#ifndef YX_UINT16_H
#define YX_UINT16_H



#include <stdint.h> /* for uint8_t, uint16_t */



/* Coordinates for maps of max. 65536x65536 cells. */
struct yx_uint16
{
    uint16_t y;
    uint16_t x;
};



/* Directions available for movement. */
enum dir
{
    NORTH = 1,
    EAST,
    SOUTH,
    WEST
};



/* Return 1 if two yx_uint16 coordinates at "a" and "b" are equal, else 0. */
extern uint8_t yx_uint16_cmp(struct yx_uint16 * a, struct yx_uint16 * b);



/* Return yx_uint16 coordinate one step from coordinate yx in direction dir.
 *
 * If an invalid dir "d" is passed, "yx" remains unchanged.
 */
extern struct yx_uint16 mv_yx_in_dir(enum dir d, struct yx_uint16 yx);



#endif
