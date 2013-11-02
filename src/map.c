#include "map.h"
#include <stdint.h>      /* for uint16_t, uint32_t */
#include "misc.h"        /* for try_malloc(), center_offset() */
#include "map_objects.h" /* for Player struct */
#include "yx_uint16.h"   /* for yx_uint16 and dir enums */
#include "rrand.h"       /* for rrand() */
#include "windows.h"     /* for struct Win */



extern struct Map init_map()
{
    char * f_name = "init_map()";
    struct Map map;
    map.size.x = 64;
    map.size.y = 64;
    uint32_t size = map.size.x * map.size.y;
    map.cells = try_malloc(size, f_name);
    uint16_t y, x;
    for (y = 0; y < map.size.y; y++)
    {
        for (x = 0; x < map.size.x; x++)
        {
            map.cells[(y * map.size.x) + x] = '~';
        }
    }
    map.cells[size / 2 + (map.size.x / 2)] = '.';
    uint32_t curpos;
    while (1)
    {
        y = rrand() % map.size.y;
        x = rrand() % map.size.x;
        curpos = y * map.size.x + x;
        if ('~' == map.cells[curpos]
            && ((curpos >= map.size.x && '.' == map.cells[curpos - map.size.x])
                || (curpos < map.size.x * (map.size.y-1)
                     && '.' == map.cells[curpos + map.size.x])
                || (curpos > 0 && curpos % map.size.x != 0
                    && '.' == map.cells[curpos-1])
                || (curpos < (map.size.x * map.size.y)
                    && (curpos+1) % map.size.x != 0
                    && '.' == map.cells[curpos+1])))
        {
            if (y == 0 || y == map.size.y - 1 || x == 0 || x == map.size.x - 1)
            {
                break;
            }
            map.cells[y * map.size.x + x] = '.';
        }
    }
    return map;
}



extern void map_scroll(struct Win * win, struct yx_uint16 map_size, enum dir d)
{
    uint16_t offset;
    if ((NORTH == d || SOUTH == d) && map_size.y > win->framesize.y)
    {
        offset = center_offset(win->center.y, map_size.y, win->framesize.y);
        win->center.y = offset + (win->framesize.y / 2);
        if      (NORTH == d && win->center.y > 0)
        {
            win->center.y--;
        }
        else if (SOUTH == d && win->center.y < map_size.y - 1)
        {
            win->center.y++;
        }
    }
    else if ((WEST == d || EAST == d) && map_size.x > win->framesize.x)
    {
        offset = center_offset(win->center.x, map_size.x, win->framesize.x);
        win->center.x = offset + (win->framesize.x / 2);
        if      (WEST == d && win->center.x > 0)
        {
            win->center.x--;
        }
        else if (EAST == d && win->center.x < map_size.x - 1)
        {
            win->center.x++;
        }
    }
}
