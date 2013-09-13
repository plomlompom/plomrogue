/* control.c */

#include "control.h"
#include <stdint.h> /* for uint8_t */
#include "windows.h" /* for cycle_active_win(), shift_active_win(), struct Win,
                      *  struct WinMeta
                      */
#include "keybindings.h" /* for get_keycode_to_action(), save_keybindings(),
                          * move_keys_mod_selection(), keyswin_mod_key()
                          */
#include "map.h" /* for map_scroll(), map_center_player(), dir enum */
#include "main.h" /* for World struct */
#include "rexit.h" /* for exit_err() */
#include "wincontrol.h" /* for scroll_pad(), toggle_window(),
                         * growshrink_active_window(), reload_win_config()
                         * toggle_winconfig(), save_win_configs(),
                         * toggle_win_height_type(), toggle_win_width_type()
                         */
#include "map_object_actions.h" /* for player_wait(), move_player() */
#include "command_db.h" /* for is_command_id_shortdsc() */



extern void record_control(int action, struct World * world)
{
    if      (is_command_id_shortdsc(world, action, "wait"))
    {
        player_wait(world);
    }
    else if (is_command_id_shortdsc(world, action, "player_u"))
    {
        move_player(world, NORTH);
    }
    else if (is_command_id_shortdsc(world, action, "player_r"))
    {
        move_player(world, EAST);
    }
    else if (is_command_id_shortdsc(world, action, "player_d"))
    {
        move_player(world, SOUTH);
    }
    else if (is_command_id_shortdsc(world, action, "player_l"))
    {
        move_player(world, WEST);
    }
}



extern uint8_t player_control(int key, struct World * world)
{
    if      (key == get_keycode_to_action(world->keybindings, "player_u"))
    {
        move_player(world, NORTH);
    }
    else if (key == get_keycode_to_action(world->keybindings, "player_r"))
    {
        move_player(world, EAST);
    }
    else if (key == get_keycode_to_action(world->keybindings, "player_d"))
    {
        move_player(world, SOUTH);
    }
    else if (key == get_keycode_to_action(world->keybindings, "player_l"))
    {
        move_player(world, WEST);
    }
    else if (key == get_keycode_to_action(world->keybindings, "wait"))
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
    struct WinMeta * win_meta = world->wmeta;
    struct Win * win_keys     = get_win_by_id(world, 'k'); /* Bad hardcoding. */
    struct Win * win_map      = get_win_by_id(world, 'm'); /* TODO: Replace.  */
    struct Win * win_info     = get_win_by_id(world, 'i'); /*                 */
    struct Win * win_log      = get_win_by_id(world, 'l'); /*                 */
    char * err_toggle = "Trouble with toggle_window() in meta_keys().";
    char * err_shift  = "Trouble with shift_active_win() in meta_keys().";
    char * err_resize = "Trouble with growshrink_active_window() in "
                        "meta_keys().";
    if (key == get_keycode_to_action(world->keybindings, "quit"))
    {
        return 1;
    }
    else if (key == get_keycode_to_action(world->keybindings, "scrl_r"))
    {
        scroll_pad(win_meta, '+');
    }
    else if (key == get_keycode_to_action(world->keybindings, "scrl_l"))
    {
        scroll_pad(win_meta, '-');
    }
    else if (key == get_keycode_to_action(world->keybindings, "to_keywin"))
    {
        exit_err(toggle_window(win_meta, win_keys), world, err_toggle);
    }
    else if (key == get_keycode_to_action(world->keybindings, "to_mapwin"))
    {
        exit_err(toggle_window(win_meta, win_map), world, err_toggle);
    }
    else if (key == get_keycode_to_action(world->keybindings, "to_infowin"))
    {
        exit_err(toggle_window(win_meta, win_info), world, err_toggle);
    }
    else if (key == get_keycode_to_action(world->keybindings, "to_logwin"))
    {
        exit_err(toggle_window(win_meta, win_log), world, err_toggle);
    }
    else if (key == get_keycode_to_action(world->keybindings, "cyc_win_f"))
    {
        cycle_active_win(win_meta, 'f');
    }
    else if (key == get_keycode_to_action(world->keybindings, "cyc_win_b"))
    {
        cycle_active_win(win_meta, 'b');
    }
    else if (key == get_keycode_to_action(world->keybindings, "shift_f"))
    {
        exit_err(shift_active_win(win_meta, 'f'), world, err_shift);
    }
    else if (key == get_keycode_to_action(world->keybindings, "shift_b"))
    {
        exit_err(shift_active_win(win_meta, 'b'), world, err_shift);
    }
    else if (key == get_keycode_to_action(world->keybindings, "grow_h"))
    {
        exit_err(growshrink_active_window(world, '*'), world, err_resize);
    }
    else if (key == get_keycode_to_action(world->keybindings, "shri_h"))
    {
        exit_err(growshrink_active_window(world, '_'), world, err_resize);
    }
    else if (key == get_keycode_to_action(world->keybindings, "grow_v"))
    {
        exit_err(growshrink_active_window(world, '+'), world, err_resize);
    }
    else if (key == get_keycode_to_action(world->keybindings, "shri_v"))
    {
        exit_err(growshrink_active_window(world, '-'), world, err_resize);
    }
    else if (key == get_keycode_to_action(world->keybindings, "save_keys"))
    {
        save_keybindings(world);
    }
    else if (key == get_keycode_to_action(world->keybindings, "keys_u"))
    {
        move_keys_mod_selection(world, 'u');
    }
    else if (key == get_keycode_to_action(world->keybindings, "keys_d"))
    {
        move_keys_mod_selection(world, 'd');
    }
    else if (key == get_keycode_to_action(world->keybindings, "keys_m"))
    {
        keyswin_mod_key(world, win_meta);
    }
    else if (key == get_keycode_to_action(world->keybindings, "map_u"))
    {
        map_scroll(world->map, NORTH, win_map->frame.size);
     }
    else if (key == get_keycode_to_action(world->keybindings, "map_d"))
    {
        map_scroll(world->map, SOUTH, win_map->frame.size);
    }
    else if (key == get_keycode_to_action(world->keybindings, "map_r"))
    {
        map_scroll(world->map, EAST, win_map->frame.size);
    }
    else if (key == get_keycode_to_action(world->keybindings, "map_l"))
    {
        map_scroll(world->map, WEST, win_map->frame.size);
    }
    else if (key == get_keycode_to_action(world->keybindings, "map_c"))
    {
        map_center_player(world->map, world->player, win_map->frame.size);
    }
    else if (key == get_keycode_to_action(world->keybindings, "reload_wins"))
    {
        reload_win_config(world);
    }
    else if (key == get_keycode_to_action(world->keybindings, "winconf"))
    {
        toggle_winconfig(world, world->wmeta->active);
    }
    else if (key == get_keycode_to_action(world->keybindings, "to_height_t"))
    {
        toggle_win_height_type(world, world->wmeta->active);
    }
    else if (key == get_keycode_to_action(world->keybindings, "to_width_t"))
    {
        toggle_win_width_type(world, world->wmeta->active);
    }
    else if (key == get_keycode_to_action(world->keybindings, "save_winconf"))
    {
        save_win_configs(world);
    }
    return 0;
}
