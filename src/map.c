#include "map.h"
#include <stdlib.h>      /* for malloc() */
#include <stdint.h>      /* for uint16_t, uint32_t */
#include "misc.h"        /* for rrand() and center_offset() */
#include "map_objects.h" /* for Player struct */
#include "yx_uint16.h"   /* for yx_uint16 and dir enums */



struct Map init_map ()
{
    struct Map map;
    map.size.x = 64;
    map.size.y = 64;
    map.offset.x = 0;
    map.offset.y = 0;
    uint32_t size = map.size.x * map.size.y;
    map.cells = malloc(size);
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
        y = rrand(0, 0) % map.size.y;
        x = rrand(0, 0) % map.size.x;
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



void map_scroll (struct Map * map, enum dir d, struct yx_uint16 win_size)
{
    if      (NORTH == d && map->offset.y > 0)
    {
        map->offset.y--;
    }
    else if (WEST  == d && map->offset.x > 0)
    {
        map->offset.x--;
    }
    else if (SOUTH == d && map->offset.y + win_size.y < map->size.y)
    {
        map->offset.y++;
    }
    else if (EAST  == d && map->offset.x + win_size.x < map->size.x)
    {
        map->offset.x++;
    }
}



void map_center_player(struct Map * map, struct Player * player,
                       struct yx_uint16 win_size)
{
    map->offset.y = center_offset (player->pos.y, map->size.y, win_size.y);
    map->offset.x = center_offset (player->pos.x, map->size.x, win_size.x);
}
