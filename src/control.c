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
#include "main.h" /* for world global */
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



extern uint16_t get_available_keycode_to_action(char * name)
{
    uint16_t keycode = get_keycode_to_action(world.kb_global.kbs, name);
    if (0 != keycode || 0 == world.wmeta->active)
    {
        return keycode;
    }
    struct WinConf * wc = get_winconf_by_win(world.wmeta->active);
    if (0 == wc->view)
    {
        keycode = get_keycode_to_action(wc->kb.kbs, name);
    }
    else if (1 == wc->view)
    {
        keycode = get_keycode_to_action(world.kb_wingeom.kbs, name);
    }
    else if (2 == wc->view)
    {
        keycode = get_keycode_to_action(world.kb_winkeys.kbs, name);
    }
    return keycode;
}



extern uint8_t player_control_by_id(int action)
{
    if      (is_command_id_shortdsc(action, "wait"))
    {
        player_wait();
    }
    else if (is_command_id_shortdsc(action, "player_u"))
    {
        move_player(NORTH);
    }
    else if (is_command_id_shortdsc(action, "player_r"))
    {
        move_player(EAST);
    }
    else if (is_command_id_shortdsc(action, "player_d"))
    {
        move_player(SOUTH);
    }
    else if (is_command_id_shortdsc(action, "player_l"))
    {
        move_player(WEST);
    }
    else if (is_command_id_shortdsc(action, "drop"))
    {
        player_drop();
    }
    else if (is_command_id_shortdsc(action, "pick"))
    {
        player_pick();
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t player_control_by_key(int key)
{
    char * action_name = get_func_to_keycode(world.kb_global.kbs, key);
    if (NULL == action_name && 0 != world.wmeta->active)
    {
        struct WinConf * wc = get_winconf_by_win(world.wmeta->active);
        action_name = get_func_to_keycode(wc->kb.kbs, key);
    }
    if (NULL != action_name)
    {
        uint8_t action_id = get_command_id(action_name);
        return player_control_by_id(action_id);
    }
    return 0;
}



extern uint8_t wingeom_control(int key)
{
    char * err_shift  = "Trouble with shift_active_win() in wingeom_control().";
    char * err_resize = "Trouble with growshrink_active_window() in "
                        "wingeom_control().";
    if      (key == get_available_keycode_to_action("to_height_t"))
    {
        toggle_win_height_type(world.wmeta->active);
    }
    else if (key == get_available_keycode_to_action("to_width_t"))
    {
        toggle_win_width_type(world.wmeta->active);
    }
    else if (key == get_available_keycode_to_action("grow_h"))
    {
        exit_err(growshrink_active_window('*'), err_resize);
    }
    else if (key == get_available_keycode_to_action("shri_h"))
    {
        exit_err(growshrink_active_window('_'), err_resize);
    }
    else if (key == get_available_keycode_to_action("grow_v"))
    {
        exit_err(growshrink_active_window('+'), err_resize);
    }
    else if (key == get_available_keycode_to_action("shri_v"))
    {
        exit_err(growshrink_active_window('-'), err_resize);
    }
    else if (key == get_available_keycode_to_action("shift_f"))
    {
        exit_err(shift_active_win(world.wmeta, 'f'), err_shift);
    }
    else if (key == get_available_keycode_to_action("shift_b"))
    {
        exit_err(shift_active_win(world.wmeta, 'b'), err_shift);
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t winkeyb_control(int key)
{
    struct WinConf * wc = get_winconf_by_win(world.wmeta->active);
    if      (key == get_available_keycode_to_action("w_keys_u"))
    {
        move_keyb_mod_selection(&wc->kb, 'u');
    }
    else if (key == get_available_keycode_to_action("w_keys_d"))
    {
        move_keyb_mod_selection(&wc->kb, 'd');
    }
    else if (key == get_available_keycode_to_action("w_keys_m"))
    {
        mod_selected_keyb(&wc->kb);
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t meta_control(int key)
{
    struct WinMeta * win_meta = world.wmeta;
    struct Win * win_map      = get_win_by_id('m');
    char * err_toggle = "Trouble with toggle_window() in meta_control().";
    if      (key == get_available_keycode_to_action("quit"))
    {
        return 1;
    }
    else if (key == get_available_keycode_to_action("winconf"))
    {
        toggle_winconfig(world.wmeta->active);
    }
    else if (key == get_available_keycode_to_action("cyc_win_f"))
    {
        cycle_active_win(world.wmeta, 'f');
    }
    else if (key == get_available_keycode_to_action("cyc_win_b"))
    {
        cycle_active_win(world.wmeta, 'b');
    }
    else if (key == get_available_keycode_to_action("scrl_r"))
    {
        scroll_pad(win_meta, '+');
    }
    else if (key == get_available_keycode_to_action("scrl_l"))
    {
        scroll_pad(win_meta, '-');
    }
    else if (key == get_available_keycode_to_action("to_a_keywin"))
    {
       exit_err(toggle_window(win_meta, get_win_by_id('k')), err_toggle);
    }
    else if (key == get_available_keycode_to_action("to_g_keywin"))
    {
        exit_err(toggle_window(win_meta, get_win_by_id('0')), err_toggle);
    }
    else if (key == get_available_keycode_to_action("to_wg_keywin"))
    {
        exit_err(toggle_window(win_meta, get_win_by_id('1')), err_toggle);
    }
    else if (key == get_available_keycode_to_action("to_wk_keywin"))
    {
        exit_err(toggle_window(win_meta, get_win_by_id('2')), err_toggle);
    }
    else if (key == get_available_keycode_to_action("to_mapwin"))
    {
        exit_err(toggle_window(win_meta, win_map), err_toggle);
    }
    else if (key == get_available_keycode_to_action("to_infowin"))
    {
        exit_err(toggle_window(win_meta, get_win_by_id('i')), err_toggle);
    }
    else if (key == get_available_keycode_to_action("to_inv"))
    {
        exit_err(toggle_window(win_meta, get_win_by_id('c')), err_toggle);
    }
    else if (key == get_available_keycode_to_action("to_logwin"))
    {
        exit_err(toggle_window(win_meta, get_win_by_id('l')), err_toggle);
    }
    else if (key == get_available_keycode_to_action("save_conf"))
    {
        save_interface_conf();
    }
    else if (key == get_available_keycode_to_action("g_keys_u"))
    {
        move_keyb_mod_selection(&world.kb_global, 'u');
    }
    else if (key == get_available_keycode_to_action("g_keys_d"))
    {
        move_keyb_mod_selection(&world.kb_global, 'd');
    }
    else if (key == get_available_keycode_to_action("g_keys_m"))
    {
        mod_selected_keyb(&world.kb_global);
    }
    else if (key == get_available_keycode_to_action("wg_keys_u"))
    {
        move_keyb_mod_selection(&world.kb_wingeom, 'u');
    }
    else if (key == get_available_keycode_to_action("wg_keys_d"))
    {
        move_keyb_mod_selection(&world.kb_wingeom, 'd');
    }
    else if (key == get_available_keycode_to_action("wg_keys_m"))
    {
        mod_selected_keyb(&world.kb_wingeom);
    }
    else if (key == get_available_keycode_to_action("wk_keys_u"))
    {
        move_keyb_mod_selection(&world.kb_winkeys, 'u');
    }
    else if (key == get_available_keycode_to_action("wk_keys_d"))
    {
        move_keyb_mod_selection(&world.kb_winkeys, 'd');
    }
    else if (key == get_available_keycode_to_action("wk_keys_m"))
    {
        mod_selected_keyb(&world.kb_winkeys);
    }
    else if (key == get_available_keycode_to_action("map_u"))
    {
        map_scroll(win_map, world.map->size, NORTH);
    }
    else if (key == get_available_keycode_to_action("map_d"))
    {
        map_scroll(win_map, world.map->size, SOUTH);
    }
    else if (key == get_available_keycode_to_action("map_r"))
    {
        map_scroll(win_map, world.map->size, EAST);
    }
    else if (key == get_available_keycode_to_action("map_l"))
    {
        map_scroll(win_map, world.map->size, WEST);
    }
    else if (key == get_available_keycode_to_action("map_c"))
    {
        struct MapObj * player = get_player();
        win_map->center = player->pos;
    }
    else if (key == get_available_keycode_to_action("inv_u"))
    {
        nav_inventory('u');
    }
    else if (key == get_available_keycode_to_action("inv_d"))
    {
        nav_inventory('d');
    }
    else if (key == get_available_keycode_to_action("reload_conf"))
    {
        unload_interface_conf();
        load_interface_conf();
    }
    else if (key == get_available_keycode_to_action("winconf"))
    {
        toggle_winconfig(world.wmeta->active);
    }
    return 0;
}
