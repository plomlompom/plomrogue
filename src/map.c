#include "map.h"
#include <stdint.h>      /* for uint8_t, uint16_t, uint32_t */
#include "misc.h"        /* for try_malloc(), center_offset(), rrand() */
#include "map_objects.h" /* for get_player() */
#include "yx_uint16.h"   /* for yx_uint16, dir enums */
#include "windows.h"     /* for struct Win */
#include "main.h"        /* for world global */
#include "wincontrol.h"  /* for get_win_by_id() */



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
        for (x = 0; x < map.size.x; map.cells[(y * map.size.x) + x] = '~', x++);
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



extern void map_scroll(char d)
{
    struct Win * win = get_win_by_id('m');
    uint16_t offset;
    if (('N' == d || 'S' == d) && world.map->size.y > win->framesize.y)
    {
        offset = center_offset(win->center.y,
                               world.map->size.y, win->framesize.y);
        win->center.y = offset + (win->framesize.y / 2);
        if ('S' == d && win->center.y < world.map->size.y - 1)
        {
            win->center.y++;
            return;
        }
        win->center.y = win->center.y - ('N' == d && win->center.y > 0);
    }
    else if (('W' == d || 'E' == d) && world.map->size.x > win->framesize.x)
    {
        offset = center_offset(win->center.x,
                               world.map->size.x, win->framesize.x);
        win->center.x = offset + (win->framesize.x / 2);
        if ('E' == d && win->center.x < world.map->size.x - 1)
        {
            win->center.x++;
            return;
        }
        win->center.x = win->center.x - ('W' == d && win->center.x > 0);
    }
}



extern void map_center()
{
    struct MapObj * player = get_player();
    struct Win * win_map   = get_win_by_id('m');
    win_map->center = player->pos;
}



extern uint8_t is_passable(struct Map * map, struct yx_uint16 pos)
{
    uint8_t passable = 0;
    if (0 <= pos.x && pos.x < map->size.x && 0 <= pos.y && pos.y < map->size.y)
    {
        passable = (('.' == map->cells[pos.y * map->size.x + pos.x]));
    }
    return passable;
}
