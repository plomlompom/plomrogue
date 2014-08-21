/* src/server/yx_uint8.c */

#include "yx_uint8.h"
#include <stdint.h> /* uint8_t, UINT8_MAX */
#include "../common/yx_uint8.h" /* yx_uint8 struct */



extern uint8_t yx_uint8_cmp(struct yx_uint8 * a, struct yx_uint8 * b)
{
    if (a->y == b->y && a->x == b->x)
    {
        return 1;
    }
    return 0;
}



extern struct yx_uint8 mv_yx_in_dir(char d, struct yx_uint8 yx)
{
    if      (d == 'e' && yx.y > 0        && (yx.x < UINT8_MAX || !(yx.y % 2)))
    {
        yx.x = yx.x + (yx.y % 2);
        yx.y--;
    }
    else if (d == 'd' && yx.x < UINT8_MAX)
    {
        yx.x++;
    }
    else if (d == 'c' && yx.y < UINT8_MAX && (yx.x < UINT8_MAX || !(yx.y % 2)))
    {
        yx.x = yx.x + (yx.y % 2);
        yx.y++;
    }
    else if (d == 'x' && yx.y < UINT8_MAX && (yx.x > 0 || yx.y % 2))
    {
        yx.x = yx.x - !(yx.y % 2);
        yx.y++;
    }
    else if (d == 's' && yx.x > 0)
    {
        yx.x--;
    }
    else if (d == 'w' && yx.y > 0         && (yx.x > 0 || yx.y % 2))
    {
        yx.x = yx.x - !(yx.y % 2);
        yx.y--;
    }
    return yx;
}
