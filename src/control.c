/* control.c */

#include "control.h"
#include <stdint.h> /* for uint8_t */
#include "windows.h" /* for cycle_active_win(), shift_active_win(), struct Win,
                      *  struct WinMeta
                      */
#include "keybindings.h" /* for get_keycode_to_action(), mod_selected_keyb(),
                          * move_keyb_mod_selection()
                          */
#include "map.h" /* for map_scroll() */
#include "main.h" /* for World struct */
#include "rexit.h" /* for exit_err() */
#include "wincontrol.h" /* for scroll_pad(), toggle_window(),
                         * growshrink_active_window(), toggle_winconfig(),
                         * toggle_win_height_type(), toggle_win_width_type()
                         */
#include "map_object_actions.h" /* for player_wait(), move_player(),
                                 * player_drop(), player_pick()
                                 */
#include "command_db.h" /* for is_command_id_shortdsc() */
#include "misc.h" /* for load_interface_conf(), unload_interface_conf(),
                   * save_interface_conf(), nav_inventory()
                   */
#include "yx_uint16.h" /* for dir enum */
#include "map_objects.h" /* for get_player() */



extern uint16_t get_available_keycode_to_action(struct World * world,
                                                char * name)
{
    uint16_t keycode = get_keycode_to_action(world->kb_global.kbs, name);
    if (0 != keycode || 0 == world->wmeta->active)
    {
        return keycode;
    }
    struct WinConf * wc = get_winconf_by_win(world, world->wmeta->active);
    if (0 == wc->view)
    {
        keycode = get_keycode_to_action(wc->kb.kbs, name);
    }
    else if (1 == wc->view)
    {
        keycode = get_keycode_to_action(world->kb_wingeom.kbs, name);
    }
    else if (2 == wc->view)
    {
        keycode = get_keycode_to_action(world->kb_winkeys.kbs, name);
    }
    return keycode;
}



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
    else if (is_command_id_shortdsc(world, action, "drop"))
    {
        player_drop(world);
    }
    else if (is_command_id_shortdsc(world, action, "pick"))
    {
        player_pick(world);
    }
}



extern uint8_t player_control(int key, struct World * world)
{
    if      (key == get_available_keycode_to_action(world, "wait"))
    {
        player_wait(world);
    }
    else if (key == get_available_keycode_to_action(world, "player_u"))
    {
        move_player(world, NORTH);
    }
    else if (key == get_available_keycode_to_action(world, "player_r"))
    {
        move_player(world, EAST);
    }
    else if (key == get_available_keycode_to_action(world, "player_d"))
    {
        move_player(world, SOUTH);
    }
    else if (key == get_available_keycode_to_action(world, "player_l"))
    {
        move_player(world, WEST);
    }
    else if (key == get_available_keycode_to_action(world, "drop"))
    {
        player_drop(world);
    }
    else if (key == get_available_keycode_to_action(world, "pick"))
    {
        player_pick(world);
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t wingeom_control(int key, struct World * world)
{
    char * err_shift  = "Trouble with shift_active_win() in wingeom_control().";
    char * err_resize = "Trouble with growshrink_active_window() in "
                        "wingeom_control().";
    if      (key == get_available_keycode_to_action(world, "to_height_t"))
    {
        toggle_win_height_type(world, world->wmeta->active);
    }
    else if (key == get_available_keycode_to_action(world, "to_width_t"))
    {
        toggle_win_width_type(world, world->wmeta->active);
    }
    else if (key == get_available_keycode_to_action(world, "grow_h"))
    {
        exit_err(growshrink_active_window(world, '*'), world, err_resize);
    }
    else if (key == get_available_keycode_to_action(world, "shri_h"))
    {
        exit_err(growshrink_active_window(world, '_'), world, err_resize);
    }
    else if (key == get_available_keycode_to_action(world, "grow_v"))
    {
        exit_err(growshrink_active_window(world, '+'), world, err_resize);
    }
    else if (key == get_available_keycode_to_action(world, "shri_v"))
    {
        exit_err(growshrink_active_window(world, '-'), world, err_resize);
    }
    else if (key == get_available_keycode_to_action(world, "shift_f"))
    {
        exit_err(shift_active_win(world->wmeta, 'f'), world, err_shift);
    }
    else if (key == get_available_keycode_to_action(world, "shift_b"))
    {
        exit_err(shift_active_win(world->wmeta, 'b'), world, err_shift);
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t winkeyb_control(int key, struct World * world)
{
    struct WinConf * wc = get_winconf_by_win(world, world->wmeta->active);
    if      (key == get_available_keycode_to_action(world, "w_keys_u"))
    {
        move_keyb_mod_selection(&wc->kb, 'u');
    }
    else if (key == get_available_keycode_to_action(world, "w_keys_d"))
    {
        move_keyb_mod_selection(&wc->kb, 'd');
    }
    else if (key == get_available_keycode_to_action(world, "w_keys_m"))
    {
        mod_selected_keyb(world, &wc->kb);
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t meta_control(int key, struct World * world)
{
    struct WinMeta * win_meta = world->wmeta;
    struct Win * win_map      = get_win_by_id(world, 'm');
    char * err_toggle = "Trouble with toggle_window() in meta_control().";
    if      (key == get_available_keycode_to_action(world, "quit"))
    {
        return 1;
    }
    else if (key == get_available_keycode_to_action(world, "winconf"))
    {
        toggle_winconfig(world, world->wmeta->active);
    }
    else if (key == get_available_keycode_to_action(world, "cyc_win_f"))
    {
        cycle_active_win(world->wmeta, 'f');
    }
    else if (key == get_available_keycode_to_action(world, "cyc_win_b"))
    {
        cycle_active_win(world->wmeta, 'b');
    }
    else if (key == get_available_keycode_to_action(world, "scrl_r"))
    {
        scroll_pad(win_meta, '+');
    }
    else if (key == get_available_keycode_to_action(world, "scrl_l"))
    {
        scroll_pad(win_meta, '-');
    }
    else if (key == get_available_keycode_to_action(world, "to_a_keywin"))
    {
        uint8_t test = toggle_window(win_meta, get_win_by_id(world, 'k'));
        exit_err(test, world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "to_g_keywin"))
    {
        uint8_t test = toggle_window(win_meta, get_win_by_id(world, '0'));
        exit_err(test, world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "to_wg_keywin"))
    {
        uint8_t test = toggle_window(win_meta, get_win_by_id(world, '1'));
        exit_err(test, world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "to_wk_keywin"))
    {
        uint8_t test = toggle_window(win_meta, get_win_by_id(world, '2'));
        exit_err(test, world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "to_mapwin"))
    {
        exit_err(toggle_window(win_meta, win_map), world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "to_infowin"))
    {
        uint8_t test = toggle_window(win_meta, get_win_by_id(world, 'i'));
        exit_err(test, world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "to_inv"))
    {
        uint8_t test = toggle_window(win_meta, get_win_by_id(world, 'c'));
        exit_err(test, world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "to_logwin"))
    {
        uint8_t test = toggle_window(win_meta, get_win_by_id(world, 'l'));
        exit_err(test, world, err_toggle);
    }
    else if (key == get_available_keycode_to_action(world, "save_conf"))
    {
        save_interface_conf(world);
    }
    else if (key == get_available_keycode_to_action(world, "g_keys_u"))
    {
        move_keyb_mod_selection(&world->kb_global, 'u');
    }
    else if (key == get_available_keycode_to_action(world, "g_keys_d"))
    {
        move_keyb_mod_selection(&world->kb_global, 'd');
    }
    else if (key == get_available_keycode_to_action(world, "g_keys_m"))
    {
        mod_selected_keyb(world, &world->kb_global);
    }
    else if (key == get_available_keycode_to_action(world, "wg_keys_u"))
    {
        move_keyb_mod_selection(&world->kb_wingeom, 'u');
    }
    else if (key == get_available_keycode_to_action(world, "wg_keys_d"))
    {
        move_keyb_mod_selection(&world->kb_wingeom, 'd');
    }
    else if (key == get_available_keycode_to_action(world, "wg_keys_m"))
    {
        mod_selected_keyb(world, &world->kb_wingeom);
    }
    else if (key == get_available_keycode_to_action(world, "wk_keys_u"))
    {
        move_keyb_mod_selection(&world->kb_winkeys, 'u');
    }
    else if (key == get_available_keycode_to_action(world, "wk_keys_d"))
    {
        move_keyb_mod_selection(&world->kb_winkeys, 'd');
    }
    else if (key == get_available_keycode_to_action(world, "wk_keys_m"))
    {
        mod_selected_keyb(world, &world->kb_winkeys);
    }
    else if (key == get_available_keycode_to_action(world, "map_u"))
    {
        map_scroll(win_map, world->map->size, NORTH);
    }
    else if (key == get_available_keycode_to_action(world, "map_d"))
    {
        map_scroll(win_map, world->map->size, SOUTH);
    }
    else if (key == get_available_keycode_to_action(world, "map_r"))
    {
        map_scroll(win_map, world->map->size, EAST);
    }
    else if (key == get_available_keycode_to_action(world, "map_l"))
    {
        map_scroll(win_map, world->map->size, WEST);
    }
    else if (key == get_available_keycode_to_action(world, "map_c"))
    {
        struct MapObj * player = get_player(world);
        win_map->center = player->pos;
    }
    else if (key == get_available_keycode_to_action(world, "inv_u"))
    {
        nav_inventory(world, 'u');
    }
    else if (key == get_available_keycode_to_action(world, "inv_d"))
    {
        nav_inventory(world, 'd');
    }
    else if (key == get_available_keycode_to_action(world, "reload_conf"))
    {
        unload_interface_conf(world);
        load_interface_conf(world);
    }
    else if (key == get_available_keycode_to_action(world, "winconf"))
    {
        toggle_winconfig(world, world->wmeta->active);
    }
    return 0;
}
