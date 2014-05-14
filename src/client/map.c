/* src/client/map.c */

#include "map.h"
#include <stdint.h> /* uint8_t */
#include "windows.h" /* struct Win, center_offset(), get_win_by_id() */
#include "world.h" /* for global world */



extern void map_scroll(char d)
{
    struct Win * win = get_win_by_id('m');
    uint16_t offset;
    if (('8' == d || '2' == d) && world.map.length > win->frame_size.y)
    {
        offset = center_offset(win->center.y,
                               world.map.length, win->frame_size.y);
        win->center.y = offset + (win->frame_size.y / 2);
        if ('2' == d && win->center.y < world.map.length - 1)
        {
            win->center.y++;
            return;
        }
        win->center.y = win->center.y - ('8' == d && win->center.y > 0);
    }
    else if (('4' == d || '6' == d) && (world.map.length*2) > win->frame_size.x)
    {
        offset = center_offset(win->center.x,
                               world.map.length*2, win->frame_size.x);
        win->center.x = offset + (win->frame_size.x / 2);
        if ('6' == d && win->center.x < (world.map.length * 2) - 1)
        {
            win->center.x++;
            return;
        }
        win->center.x = win->center.x - ('4' == d && win->center.x > 0);
    }
}



extern void map_center()
{
    struct Win * win_map = get_win_by_id('m');
    win_map->center.y = world.player_pos.y;
    win_map->center.x = world.player_pos.x * 2;
}
