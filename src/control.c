/* control.c */

#include "control.h"
#include <stdint.h> /* for uint8_t */
#include "windows.h" /* for cycle_active_win(), shift_active_win(), struct Win,
                      *  struct WinMeta
                      */
#include "keybindings.h" /* for get_action_key(), save_keybindings(),
                          * keyswin_move_selection(), keyswin_mod_key()
                          */
#include "map.h" /* for map_scroll(), map_center_player(), dir enum */
#include "main.h" /* for World struct */
#include "rexit.h" /* for exit_err() */
#include "misc.h" /* for scroll_pad(), toggle_window(),
                   * growshrink_active_window()
                   */
#include "map_object_actions.h" /* for player_wait(), move_player() */



extern void record_control(int action, struct World * world)
{
    if (0 == action)
    {
        player_wait(world);
    }
    else if (NORTH == action)
    {
        move_player(world, NORTH);
    }
    else if (EAST  == action)
    {
        move_player(world, EAST);
    }
    else if (SOUTH == action)
    {
        move_player(world, SOUTH);
    }
    else if (WEST == action)
    {
        move_player(world, WEST);
    }
}



extern uint8_t player_control(int key, struct World * world)
{
    if      (key == get_action_key(world->keybindings, "player up"))
    {
        move_player(world, NORTH);
    }
    else if (key == get_action_key(world->keybindings, "player right"))
    {
        move_player(world, EAST);
    }
    else if (key == get_action_key(world->keybindings, "player down"))
    {
        move_player(world, SOUTH);
    }
    else if (key == get_action_key(world->keybindings, "player left"))
    {
        move_player(world, WEST);
    }
    else if (key == get_action_key(world->keybindings, "wait / next turn"))
    {
        player_wait(world);
    }
    else
    {
        return 1;
    }
    return 0;
}



extern uint8_t meta_control(int key, struct World * world)
{
    struct WinMeta * win_meta = world->wins.meta;
    struct Win * win_keys     = world->wins.keys;
    struct Win * win_map      = world->wins.map;
    struct Win * win_info     = world->wins.info;
    struct Win * win_log      = world->wins.log;
    char * err_toggle = "Trouble with toggle_window() in meta_keys().";
    char * err_shift  = "Trouble with shift_active_win() in meta_keys().";
    char * err_resize = "Trouble with growshrink_active_window() in "
                        "meta_keys().";
    if (key == get_action_key(world->keybindings, "quit"))
    {
        return 1;
    }
    else if (key == get_action_key(world->keybindings, "scroll pad right"))
    {
        scroll_pad(win_meta, '+');
    }
    else if (key == get_action_key(world->keybindings, "scroll pad left"))
    {
        scroll_pad(win_meta, '-');
    }
    else if (key == get_action_key(world->keybindings, "toggle keys window"))
    {
        exit_err(toggle_window(win_meta, win_keys), world, err_toggle);
    }
    else if (key == get_action_key(world->keybindings, "toggle map window"))
    {
        exit_err(toggle_window(win_meta, win_map), world, err_toggle);
    }
    else if (key == get_action_key(world->keybindings, "toggle info window"))
    {
        exit_err(toggle_window(win_meta, win_info), world, err_toggle);
    }
    else if (key == get_action_key(world->keybindings, "toggle log window"))
    {
        exit_err(toggle_window(win_meta, win_log), world, err_toggle);
    }
    else if (key == get_action_key(world->keybindings, "cycle forwards"))
    {
        cycle_active_win(win_meta, 'f');
    }
    else if (key == get_action_key(world->keybindings, "cycle backwards"))
    {
        cycle_active_win(win_meta, 'b');
    }
    else if (key == get_action_key(world->keybindings, "shift forwards"))
    {
        exit_err(shift_active_win(win_meta, 'f'), world, err_shift);
    }
    else if (key == get_action_key(world->keybindings, "shift backwards"))
    {
        exit_err(shift_active_win(win_meta, 'b'), world, err_shift);
    }
    else if (key == get_action_key(world->keybindings, "grow horizontally"))
    {
        exit_err(growshrink_active_window(win_meta, '*'), world, err_resize);
    }
    else if (key == get_action_key(world->keybindings, "shrink horizontally"))
    {
        exit_err(growshrink_active_window(win_meta, '_'), world, err_resize);
    }
    else if (key == get_action_key(world->keybindings, "grow vertically"))
    {
        exit_err(growshrink_active_window(win_meta, '+'), world, err_resize);
    }
    else if (key == get_action_key(world->keybindings, "shrink vertically"))
    {
        exit_err(growshrink_active_window(win_meta, '-'), world, err_resize);
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
        map_scroll(world->map, NORTH, win_map->frame.size);
     }
    else if (key == get_action_key(world->keybindings, "map down"))
    {
        map_scroll(world->map, SOUTH, win_map->frame.size);
    }
    else if (key == get_action_key(world->keybindings, "map right"))
    {
        map_scroll(world->map, EAST, win_map->frame.size);
    }
    else if (key == get_action_key(world->keybindings, "map left"))
    {
        map_scroll(world->map, WEST, win_map->frame.size);
    }
    else if (key == get_action_key(world->keybindings, "map center player"))
    {
        map_center_player (world->map, world->player, win_map->frame.size);
    }
    return 0;
}
