/* yx_uint16.c */



#include "yx_uint16.h" /* for uint8_t, uint16_t */



extern uint8_t yx_uint16_cmp(struct yx_uint16 * a, struct yx_uint16 * b)
{
    if (a->y == b->y && a->x == b->x)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



extern struct yx_uint16 mv_yx_in_dir(enum dir d, struct yx_uint16 yx)
{
    if      (d == NORTH)
    {
        yx.y--;
    }
    else if (d == EAST)
    {
        yx.x++;
    }
    else if (d == SOUTH)
    {
        yx.y++;
    }
    else if (d == WEST)
    {
        yx.x--;
    }
    return yx;
}
