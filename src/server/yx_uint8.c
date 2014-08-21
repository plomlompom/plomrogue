/* src/server/yx_uint8.c */

#include "yx_uint8.h"
#include <stdint.h> /* uint8_t, int8_t */
#include <string.h> /* strchr() */
#include "../common/yx_uint8.h" /* yx_uint8 */



/* Move "yx" into hex direction "d". */
static void mv_yx_in_hex_dir(char d, struct yx_uint8 * yx);



static void mv_yx_in_hex_dir(char d, struct yx_uint8 * yx)
{
    if     (d == 'e')
    {
        yx->x = yx->x + (yx->y % 2);
        yx->y--;
    }
    else if (d == 'd')
    {
        yx->x++;
    }
    else if (d == 'c')
    {
        yx->x = yx->x + (yx->y % 2);
        yx->y++;
    }
    else if (d == 'x')
    {
        yx->x = yx->x - !(yx->y % 2);
        yx->y++;
    }
    else if (d == 's')
    {
        yx->x--;
    }
    else if (d == 'w')
    {
        yx->x = yx->x - !(yx->y % 2);
        yx->y--;
    }
}



extern uint8_t mv_yx_in_dir_wrap(char d, struct yx_uint8 * yx, uint8_t unwrap)
{
    static int8_t wrap_west_east   = 0;
    static int8_t wrap_north_south = 0;
    if (unwrap)
    {
        wrap_west_east = wrap_north_south = 0;
        return 0;
    }
    struct yx_uint8 original;
    original.y = yx->y;
    original.x = yx->x;
    mv_yx_in_hex_dir(d, yx);
    if      (strchr("edc", d) && yx->x < original.x)
    {
        wrap_west_east++;
    }
    else if (strchr("xsw", d) && yx->x > original.x)
    {
        wrap_west_east--;
    }
    if      (strchr("we", d) && yx->y > original.y)
    {
        wrap_north_south--;
    }
    else if (strchr("xc", d) && yx->y < original.y)
    {
        wrap_north_south++;
    }
    return (wrap_west_east != 0) + (wrap_north_south != 0);
}
