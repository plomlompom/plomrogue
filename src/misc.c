/* misc.c */

#include "misc.h"
#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strcmp(), memcpy() */
#include <stdint.h> /* for uint8_t */
#include "windows.h" /* for suspend_win(), append_win(), reset_pad_offset(),
                      * resize_active_win(), cycle_active_win(),
                      * shift_active_win(), struct Win, struct WinMeta
                      */
#include "keybindings.h" /* for get_action_key(), save_keybindings(),
                          * keyswin_move_selection(), keyswin_mod_key()
                          */
#include "readwrite.h" /* for [read/write]_uint[8/16/32][_bigendian]() */
#include "map_objects.h" /* for struct Monster, write_map_objects(), */
#include "map_object_actions.h" /* for is_passable(), move_monster() */
#include "map.h" /* for map_scroll(),map_center_player(), Map struct,dir enum */
#include "main.h" /* for World struct */
#include "yx_uint16.h" /* for yx_uint16 */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_err() */


extern void textfile_sizes(FILE * file, uint16_t * linemax_p,
                           uint16_t * n_lines_p)
{
    uint16_t n_lines = 0;
    int c = 0;
    uint16_t linemax = 0;
    uint16_t c_count = 0;
    while (EOF != c)
    {
        c_count++;
        c = getc(file);
        if ('\n' == c)
        {
            if (c_count > linemax)
            {
                linemax = c_count + 1;
            }
            c_count = 0;
            if (n_lines_p)
            {
                n_lines++;
            }
        }
    }
    fseek(file, 0, SEEK_SET);
    * linemax_p = linemax;
    if (n_lines_p)
    {
        * n_lines_p = n_lines;
    }
}



extern void update_log(struct World * world, char * text)
{
    static char * last_msg;
    if (0 == last_msg)
    {
        last_msg = calloc(1, sizeof(char));
    }
    char * new_text;
    uint16_t len_old = strlen(world->log);
    if (0 == strcmp(last_msg, text))
    {
        uint16_t len_whole = len_old + 1;
        new_text = calloc(len_whole + 1, sizeof(char));
        memcpy(new_text, world->log, len_old);
        memcpy(new_text + len_old, ".", 1);
    }
    else
    {
        uint16_t len_new = strlen(text);
        uint16_t len_whole = len_old + len_new + 1;
        new_text = calloc(len_whole, sizeof(char));
        memcpy(new_text, world->log, len_old);
        memcpy(new_text + len_old, text, len_new);
        last_msg = calloc(len_new + 1, sizeof(char));
        memcpy(last_msg, text, len_new);
    }
    free(world->log);
    world->log = new_text;
}



extern uint16_t center_offset(uint16_t pos, uint16_t mapsize,
                              uint16_t framesize)
{
    uint16_t offset = 0;
    if (mapsize > framesize)
    {
        if (pos > framesize / 2)
        {
            if (pos < mapsize - (framesize / 2))
            {
                offset = pos - (framesize / 2);
            }
            else
            {
                offset = mapsize - framesize;
            }
        }
    }
    return offset;
}



extern void turn_over(struct World * world, char action)
{
    if (1 == world->interactive)
    {
        FILE * file = fopen("record", "a");
        exit_err(write_uint8(action, file), world, "Record writing failure.");
        fclose(file);
    }
    world->turn++;
    rrand_seed(world->seed * world->turn);
    struct Monster * monster;
    for (monster = world->monster;
         monster != 0;
         monster = monster->map_obj.next)
    {
        move_monster(world, monster);
    }
}



extern void save_game(struct World * world)
{
    uint8_t err = 0;
    char * err_msg = "Error saving game.";

    FILE * file = fopen("savefile", "w");
    // err = (0 == file);
    exit_err(0 == file, world, err_msg);

    err = write_uint32_bigendian(world->seed, file);
    err = err | write_uint32_bigendian(world->turn, file);
    err = err | write_uint16_bigendian(world->player->pos.y + 1, file);
    err = err | write_uint16_bigendian(world->player->pos.x + 1, file);
    err = err | write_uint8(world->player->hitpoints, file);
    err = err | write_map_objects(world, world->monster, file);
    err = err | write_map_objects(world, world->item, file);
    exit_err(err, world, err_msg);
    fclose(file);
}



extern void toggle_window(struct WinMeta * win_meta, struct Win * win)
{
    if (0 != win->frame.curses_win)
    {
        suspend_win(win_meta, win);
    }
    else
    {
        append_win(win_meta, win);
    }
}



extern void scroll_pad(struct WinMeta * win_meta, char dir)
{
    if      ('+' == dir)
    {
        reset_pad_offset(win_meta, win_meta->pad_offset + 1);
    }
    else if ('-' == dir)
    {
        reset_pad_offset(win_meta, win_meta->pad_offset - 1);
    }
}



extern void growshrink_active_window(struct WinMeta * win_meta, char change)
{
    if (0 != win_meta->active)
    {
        struct yx_uint16 size = win_meta->active->frame.size;
        if      (change == '-')
        {
            size.y--;
        }
        else if (change == '+')
        {
            size.y++;
        }
        else if (change == '_')
        {
            size.x--;
        }
        else if (change == '*')
        {
            size.x++;
        }
        resize_active_win (win_meta, size);
    }
}



extern struct yx_uint16 find_passable_pos(struct Map * map)
{
    struct yx_uint16 pos;
    for (pos.y = pos.x = 0; 0 == is_passable(map, pos);)
    {
        pos.y = rrand() % map->size.y;
        pos.x = rrand() % map->size.x;
    }
    return pos;
}



extern uint8_t meta_keys(int key, struct World * world,
                         struct WinMeta * win_meta, struct Win * win_keys,
                         struct Win * win_map, struct Win * win_info,
                         struct Win * win_log)
{
    if (key == get_action_key(world->keybindings, "quit"))
    {
        return 1;
    }
    else if (key == get_action_key(world->keybindings, "scroll pad right"))
    {
        scroll_pad (win_meta, '+');
    }
    else if (key == get_action_key(world->keybindings, "scroll pad left"))
    {
        scroll_pad (win_meta, '-');
    }
    else if (key == get_action_key(world->keybindings, "toggle keys window"))
    {
        toggle_window(win_meta, win_keys);
    }
    else if (key == get_action_key(world->keybindings, "toggle map window"))
    {
        toggle_window(win_meta, win_map);
    }
    else if (key == get_action_key(world->keybindings, "toggle info window"))
    {
        toggle_window(win_meta, win_info);
    }
    else if (key == get_action_key(world->keybindings, "toggle log window"))
    {
        toggle_window(win_meta, win_log);
    }
    else if (key == get_action_key(world->keybindings, "cycle forwards"))
    {
        cycle_active_win(win_meta, 'n');
    }
    else if (key == get_action_key(world->keybindings, "cycle backwards"))
    {
        cycle_active_win(win_meta, 'p');
    }
    else if (key == get_action_key(world->keybindings, "shift forwards"))
    {
        shift_active_win(win_meta, 'f');
    }
    else if (key == get_action_key(world->keybindings, "shift backwards"))
    {
        shift_active_win(win_meta, 'b');
    }
    else if (key == get_action_key(world->keybindings, "grow horizontally"))
    {
        growshrink_active_window(win_meta, '*');
    }
    else if (key == get_action_key(world->keybindings, "shrink horizontally"))
    {
        growshrink_active_window(win_meta, '_');
    }
    else if (key == get_action_key(world->keybindings, "grow vertically"))
    {
        growshrink_active_window(win_meta, '+');
    }
    else if (key == get_action_key(world->keybindings, "shrink vertically"))
    {
        growshrink_active_window(win_meta, '-');
    }
    else if (key == get_action_key(world->keybindings, "save keys"))
    {
        save_keybindings(world);
    }
    else if (key == get_action_key(world->keybindings, "keys nav up"))
    {
        keyswin_move_selection (world, 'u');
    }
    else if (key == get_action_key(world->keybindings, "keys nav down"))
    {
        keyswin_move_selection (world, 'd');
    }
    else if (key == get_action_key(world->keybindings, "keys mod"))
    {
        keyswin_mod_key (world, win_meta);
    }
    else if (key == get_action_key(world->keybindings, "map up"))
    {
        map_scroll (world->map, NORTH, win_map->frame.size);
     }
    else if (key == get_action_key(world->keybindings, "map down"))
    {
        map_scroll (world->map, SOUTH, win_map->frame.size);
    }
    else if (key == get_action_key(world->keybindings, "map right"))
    {
        map_scroll (world->map, EAST, win_map->frame.size);
    }
    else if (key == get_action_key(world->keybindings, "map left"))
    {
        map_scroll (world->map, WEST, win_map->frame.size);
    }
    else if (key == get_action_key(world->keybindings, "map center player"))
    {
        map_center_player (world->map, world->player, win_map->frame.size);
    }
    return 0;
}
