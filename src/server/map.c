/* src/server/map.c */

#include "map.h"
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* struct yx_uint8 */
#include "rrand.h" /* rrand() */
#include "world.h" /* global world */



extern void init_map()
{
    char * f_name = "init_map()";
    uint32_t size = world.map.size.x * world.map.size.y;
    world.map.cells = try_malloc(size, f_name);
    uint16_t y, x;
    for (y = 0; y < world.map.size.y; y++)
    {
        for (x = 0;
             x < world.map.size.x;
             world.map.cells[(y * world.map.size.x) + x] = '~', x++);
    }
    uint8_t add_half_width = !(world.map.size.y % 2) * (world.map.size.x / 2);
    world.map.cells[(size / 2) + add_half_width] = '.';
    struct yx_uint8 pos;
    uint16_t posi;
    char * err = "Map generation reached iteration limit. Change map size?";
    uint32_t i;
    for (i = 0; ; i++, exit_err(256 * UINT16_MAX == i, err))
    {
        pos.y = rrand() % world.map.size.y;
        pos.x = rrand() % world.map.size.x;
        posi = (pos.y * world.map.size.x) + pos.x;
        uint8_t ind = pos.y % 2;
        uint8_t diag_west = pos.x + ind > 0;
        uint8_t diag_east = pos.x + ind <= world.map.size.x - 1;
        if ('~' == world.map.cells[posi]
            && (   (   pos.y > 0                    && diag_east
                    && '.' == world.map.cells[posi - world.map.size.x + ind])
                || (   pos.x < world.map.size.x - 1
                    && '.' == world.map.cells[posi + 1])
                || (   pos.y < world.map.size.y - 1 && diag_east
                    && '.' == world.map.cells[posi + world.map.size.x + ind])
                || (   pos.y > 0                    && diag_west
                    && '.' == world.map.cells[posi - world.map.size.x - !ind])
                || (   pos.x > 0
                    && '.' == world.map.cells[posi - 1])
                || (   pos.y < world.map.size.y - 1 && diag_west
                    && '.' == world.map.cells[posi + world.map.size.x - !ind])))
        {
            if (   pos.y == 0 || pos.y == world.map.size.y - 1
                || pos.x == 0 || pos.x == world.map.size.x - 1)
            {
                break;
            }
            world.map.cells[posi] = '.';
        }
    }
}



extern uint8_t is_passable(struct yx_uint8 pos)
{
    uint8_t passable = 0;
    if (pos.x < world.map.size.x && pos.y < world.map.size.y)
    {
        passable = ('.' == world.map.cells[(pos.y * world.map.size.x) + pos.x]);
    }
    return passable;
}
