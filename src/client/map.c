/* src/client/map.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "map.h"
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* free() */
#include <stdio.h> /* sprintf() */
#include <string.h> /* strlen(), strncmp() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/yx_uint8.h" /* yx_uint8 */
#include "io.h" /* send() */
#include "windows.h" /* struct Win, center_offset(), get_win_by_id() */
#include "world.h" /* for global world */



extern void map_scroll(char d)
{
    world.autofocus = 0;
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
    struct yx_uint8 pos = world.look ? world.look_pos : world.player_pos;
    win_map->center.y = pos.y;
    win_map->center.x = pos.x * 2 + (pos.y % 2);
}



extern void toggle_autofocus()
{
    world.autofocus = world.autofocus ? 0 : 1;
}



extern void toggle_lookmode()
{
    if (!world.look)
    {
        world.look_pos = world.player_pos;
        world.look = 1;
    }
    else
    {
        world.look = 0;
    }
    query_mapcell();
}



extern uint8_t lookmode_nav(char * command)
{
    char * prefix = "move_";
    uint8_t len_pref = strlen(prefix);
    if (!strncmp(prefix, command, len_pref) && strlen(command) - 1 == len_pref)
    {
        uint8_t open_north = world.look_pos.y > 0;
        uint8_t open_south = world.look_pos.y < world.map.length - 1;
        uint8_t open_west  = world.look_pos.x > 0;
        uint8_t open_east  = world.look_pos.x < world.map.length - 1;
        uint8_t indent     = world.look_pos.y % 2;
        if      ('s' == command[len_pref])
        {
            world.look_pos.x = world.look_pos.x - open_west;
        }
        else if ('d' == command[len_pref])
        {
            world.look_pos.x = world.look_pos.x + open_east;
        }
        else if ('w' == command[len_pref])
        {
            world.look_pos.y = world.look_pos.y - open_north;
            world.look_pos.x = world.look_pos.x - !indent * open_west;
        }
        else if ('e' == command[len_pref])
        {
            world.look_pos.y = world.look_pos.y - open_north;
            world.look_pos.x = world.look_pos.x + indent * open_east;
        }
        else if ('x' == command[len_pref])
        {
            world.look_pos.y = world.look_pos.y + open_south;
            world.look_pos.x = world.look_pos.x - !indent * open_west;
        }
        else if ('c' == command[len_pref])
        {
            world.look_pos.y = world.look_pos.y + open_south;
            world.look_pos.x = world.look_pos.x + indent * open_east;
        }
        else
        {
            return 0;
        }
        map_center();
        query_mapcell();
        return 1;
    }
    return 0;
}



extern void query_mapcell()
{
    free(world.things_here);
    world.things_here = NULL;
    char * stack = "THINGS_HERE";
    char * stack_query = try_malloc(strlen(stack) +1+3 +1+3 +1, __func__);
    uint8_t y = world.look ? world.look_pos.y : world.player_pos.y;
    uint8_t x = world.look ? world.look_pos.x : world.player_pos.x;
    int test = sprintf(stack_query, "%s %d %d", stack, y, x);
    exit_trouble(test < 0, __func__, "sprintf");
    send(stack_query);
    free(stack_query);
}
