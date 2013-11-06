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



extern struct yx_uint16 mv_yx_in_dir(char d, struct yx_uint16 yx)
{
    if      (d == 'N')
    {
        yx.y--;
    }
    else if (d == 'E')
    {
        yx.x++;
    }
    else if (d == 'S')
    {
        yx.y++;
    }
    else if (d == 'W')
    {
        yx.x--;
    }
    return yx;
}
