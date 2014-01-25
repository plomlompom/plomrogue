/* src/client/map_window.c */

#include "map_window.h"
#include <stdint.h> /* uint16_t */
#include "misc.h" /* center_offset() */
#include "windows.h" /* struct Win, get_win_by_id() */
#include "world.h" /* for global world */



extern void map_scroll(char d)
{
    struct Win * win = get_win_by_id('m');
    uint16_t offset;
    if (('N' == d || 'S' == d) && world.map.size.y > win->frame_size.y)
    {
        offset = center_offset(win->center.y,
                               world.map.size.y, win->frame_size.y);
        win->center.y = offset + (win->frame_size.y / 2);
        if ('S' == d && win->center.y < world.map.size.y - 1)
        {
            win->center.y++;
            return;
        }
        win->center.y = win->center.y - ('N' == d && win->center.y > 0);
    }
    else if (('W' == d || 'E' == d) && world.map.size.x > win->frame_size.x)
    {
        offset = center_offset(win->center.x,
                               world.map.size.x, win->frame_size.x);
        win->center.x = offset + (win->frame_size.x / 2);
        if ('E' == d && win->center.x < world.map.size.x - 1)
        {
            win->center.x++;
            return;
        }
        win->center.x = win->center.x - ('W' == d && win->center.x > 0);
    }
}



extern void map_center()
{
    struct Win * win_map = get_win_by_id('m');
    win_map->center = world.player_pos;
}
