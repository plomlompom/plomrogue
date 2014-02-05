/* src/server/yx_uint16.c */

#include "yx_uint16.h"
#include <stdint.h> /* uint8_t, UINT16_MAX */
#include "../common/yx_uint16.h" /* yx_uint16 struct */



extern uint8_t yx_uint16_cmp(struct yx_uint16 * a, struct yx_uint16 * b)
{
    if (a->y == b->y && a->x == b->x)
    {
        return 1;
    }
    return 0;
}



extern struct yx_uint16 mv_yx_in_dir(char d, struct yx_uint16 yx)
{
    if      (d == '8' && yx.y > 0)
    {
        yx.y--;
    }
    else if (d == '9' && yx.y > 0 && yx.x < UINT16_MAX)
    {
        yx.y--;
        yx.x++;
    }
    else if (d == '6' && yx.x < UINT16_MAX)
    {
        yx.x++;
    }
    else if (d == '3' && yx.x < UINT16_MAX && yx.y < UINT16_MAX)
    {
        yx.y++;
        yx.x++;
    }
    else if (d == '2' && yx.y < UINT16_MAX)
    {
        yx.y++;
    }
    else if (d == '1' && yx.y < UINT16_MAX && yx.x > 0)
    {
        yx.y++;
        yx.x--;
    }
    else if (d == '4' && yx.x > 0)
    {
        yx.x--;
    }
    else if (d == '7' && yx.x > 0 && yx.y > 0)
    {
        yx.y--;
        yx.x--;
    }
    return yx;
}
