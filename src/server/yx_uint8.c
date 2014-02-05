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
    if      (d == '8' && yx.y > 0)
    {
        yx.y--;
    }
    else if (d == '9' && yx.y > 0 && yx.x < UINT8_MAX)
    {
        yx.y--;
        yx.x++;
    }
    else if (d == '6' && yx.x < UINT8_MAX)
    {
        yx.x++;
    }
    else if (d == '3' && yx.x < UINT8_MAX && yx.y < UINT8_MAX)
    {
        yx.y++;
        yx.x++;
    }
    else if (d == '2' && yx.y < UINT8_MAX)
    {
        yx.y++;
    }
    else if (d == '1' && yx.y < UINT8_MAX && yx.x > 0)
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
