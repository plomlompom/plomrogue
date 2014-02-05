/* src/server/map.c */

#include "map.h"
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint16.h" /* struct yx_uint16 */
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
    world.map.cells[size / 2 + (world.map.size.x / 2)] = '.';
    uint32_t curpos;
    while (1)
    {
        y = rrand() % world.map.size.y;
        x = rrand() % world.map.size.x;
        curpos = y * world.map.size.x + x;
        if ('~' == world.map.cells[curpos]
            && (   (   curpos >= world.map.size.x
                    && '.' == world.map.cells[curpos - world.map.size.x])
                || (   curpos < world.map.size.x * (world.map.size.y-1)
                    && '.' == world.map.cells[curpos + world.map.size.x])
                || (   curpos > 0 && curpos % world.map.size.x != 0
                    && '.' == world.map.cells[curpos-1])
                || (   curpos < (world.map.size.x * world.map.size.y)
                    && (curpos+1) % world.map.size.x != 0
                    && '.' == world.map.cells[curpos+1])))
        {
            if (  y == 0 || y == world.map.size.y - 1 || x == 0
                || x == world.map.size.x - 1)
            {
                break;
            }
            world.map.cells[y * world.map.size.x + x] = '.';
        }
    }
}



extern uint8_t is_passable(struct yx_uint16 pos)
{
    uint8_t passable = 0;
    if (   0 <= pos.x && pos.x < world.map.size.x
        && 0 <= pos.y && pos.y < world.map.size.y)
    {
        passable = (('.' == world.map.cells[pos.y * world.map.size.x + pos.x]));
    }
    return passable;
}
