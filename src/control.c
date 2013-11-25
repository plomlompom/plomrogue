/* control.c */

#include "control.h"
#include <stdint.h> /* for uint8_t */
#include <string.h> /* for strcmp() */
#include "windows.h" /* for cycle_active_win(), shift_active_win(), struct Win,
                      *  struct WinMeta
                      */
#include "keybindings.h" /* for get_keycode_to_action(), mod_selected_keyb(),
                          * move_keyb_mod_selection(), get_func_to_keycode()
                          */
#include "map.h" /* for map_scroll(), map_center() */
#include "main.h" /* for world global */
#include "wincontrol.h" /* for struct WinConf, scroll_pad(), toggle_window(),
                         * growshrink_active_window(),toggle_winconfig(),
                         * toggle_size_type()
                         */
#include "map_object_actions.h" /* for struct MapObjAct, actor_wait(),
                                 * actor_move(), actor_drop(), actor_pick(),
                                 * actor_pick(), get_moa_id_by_name()
                                 */
#include "command_db.h" /* for is_command_id_shortdsc() */
#include "misc.h" /* for reload_interface_conf(), save_interface_conf(),
                   * nav_inventory(), turn_over()
                   */
#include "map_objects.h" /* for get_player() */



/* If "cmd" matches "match" in get_available_keycode_to_action(), execute "f"
 * with provided char arguments and return 1; else only return 0.
 */
static uint8_t try_cmd_0args(int cmd, char * match, void (* f) ());
static uint8_t try_cmd_1args(int cmd, char * match, void (* f) (char), char c);
static uint8_t try_cmd_2args(int cmd, char * match,
                             void (* f) (char, char), char c1, char c2);

/* If "action" is id of command named "match", set player->arg, ->command and
 * call turn_over().
 */
static uint8_t try_player_cmd(int action, char * match, char * command,
                              uint8_t arg);

/* Return pointer to global keybindings or to keybindings for wingeometry config
 * (c = "g") or winkeys config (c = "k") or active window's keybindings ("w").
 */
static struct KeyBiData * select_keybidata_pointer(char c);

/* Wrappers to make some functions compatible to try_cmd_* single char args. */
static void wrap_mod_selected_keyb(char c);
static void wrap_mv_kb_mod(char c1, char c2);



static uint8_t try_cmd_0args(int cmd, char * match, void (* f) ())
{
    if (cmd == get_available_keycode_to_action(match))
    {
        f();
        return 1;
    }
    return 0;
}



static uint8_t try_cmd_1args(int cmd, char * match, void (* f) (char), char c)
{
    if (cmd == get_available_keycode_to_action(match))
    {
        f(c);
        return 1;
    }
    return 0;
}



static uint8_t try_cmd_2args(int cmd, char * match,
                             void (* f) (char, char), char c1, char c2)
{
    if (cmd == get_available_keycode_to_action(match))
    {
        f(c1, c2);
        return 1;
    }
    return 0;
}



static uint8_t try_player_cmd(int action, char * match, char * command,
                              uint8_t arg)
{
    if (is_command_id_shortdsc(action, match))
    {
        struct MapObj * player = get_player();
        player->arg = arg;
        player->command = get_moa_id_by_name(command);
        turn_over(get_command_id(match));
        return 1;
    }
    return 0;
}



static struct KeyBiData * select_keybidata_pointer(char c)
{
    struct KeyBiData * kbd;
    kbd = &world.kb_global;
    if      ('g' == c)
    {
        kbd = &world.kb_wingeom;
    }
    else if ('k' == c)
    {
        kbd = &world.kb_winkeys;
    }
    else if ('w' == c)
    {
        struct WinConf * wc = get_winconf_by_win(world.wmeta->active);
        kbd = &wc->kb;
    }
    return kbd;
}



static void wrap_mod_selected_keyb(char c)
{
        mod_selected_keyb(select_keybidata_pointer(c));
}



static void wrap_mv_kb_mod(char c1, char c2)
{
        move_keyb_mod_selection(select_keybidata_pointer(c1), c2);
}



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




extern uint8_t player_control_by_id(int action)
{
    if (   try_player_cmd(action, "wait", "wait", 0)
        || try_player_cmd(action, "drop", "drop", world.inventory_select)
        || try_player_cmd(action, "pick", "pick_up", 0)
        || try_player_cmd(action, "use", "use", world.inventory_select)
        || try_player_cmd(action, "player_u", "move", 'N')
        || try_player_cmd(action, "player_d", "move", 'S')
        || try_player_cmd(action, "player_r", "move", 'E')
        || try_player_cmd(action, "player_l", "move", 'W'))
    {
        return 1;
    }
    return 0;
}



extern uint8_t wingeom_control(int key)
{
    if (   try_cmd_1args(key, "to_height_t", toggle_win_size_type, 'y')
        || try_cmd_1args(key, "to_width_t", toggle_win_size_type, 'x')
        || try_cmd_1args(key, "grow_h", growshrink_active_window, '*')
        || try_cmd_1args(key, "shri_h", growshrink_active_window, '_')
        || try_cmd_1args(key, "grow_v", growshrink_active_window, '+')
        || try_cmd_1args(key, "shri_v", growshrink_active_window, '-')
        || try_cmd_1args(key, "shift_f", shift_active_win, 'f')
        || try_cmd_1args(key, "shift_b", shift_active_win, 'b'))
    {
        return 1;
    }
    return 0;
}



extern uint8_t winkeyb_control(int key)
{
    if (   try_cmd_1args(key, "w_keys_m", wrap_mod_selected_keyb, 'w')
        || try_cmd_2args(key, "w_keys_u", wrap_mv_kb_mod, 'w', 'u')
        || try_cmd_2args(key, "w_keys_d", wrap_mv_kb_mod, 'w', 'd'))
    {
        return 1;
    }
    return 0;
}



extern uint8_t meta_control(int key)
{
    uint8_t ret = (key == get_available_keycode_to_action("quit"));
    if (   (0 == ret)
        && (   try_cmd_0args(key, "winconf", toggle_winconfig)
            || try_cmd_0args(key, "reload_conf", reload_interface_conf)
            || try_cmd_0args(key, "save_conf", save_interface_conf)
            || try_cmd_0args(key, "map_c", map_center)
            || try_cmd_1args(key, "scrl_r", scroll_pad, '+')
            || try_cmd_1args(key, "scrl_l", scroll_pad, '-')
            || try_cmd_1args(key, "to_a_keywin", toggle_window, 'k')
            || try_cmd_1args(key, "to_g_keywin", toggle_window, '0')
            || try_cmd_1args(key, "to_wg_keywin", toggle_window, '1')
            || try_cmd_1args(key, "to_wk_keywin", toggle_window, '2')
            || try_cmd_1args(key, "to_mapwin", toggle_window, 'm')
            || try_cmd_1args(key, "to_infowin", toggle_window, 'i')
            || try_cmd_1args(key, "to_inv", toggle_window, 'c')
            || try_cmd_1args(key, "to_logwin", toggle_window, 'l')
            || try_cmd_1args(key, "cyc_win_f", cycle_active_win, 'f')
            || try_cmd_1args(key, "cyc_win_b", cycle_active_win, 'b')
            || try_cmd_1args(key, "g_keys_m", wrap_mod_selected_keyb, 'G')
            || try_cmd_1args(key, "wg_keys_m", wrap_mod_selected_keyb, 'g')
            || try_cmd_1args(key, "wk_keys_m", wrap_mod_selected_keyb, 'k')
            || try_cmd_1args(key, "inv_u", nav_inventory, 'u')
            || try_cmd_1args(key, "inv_d", nav_inventory, 'd')
            || try_cmd_1args(key, "map_u", map_scroll, 'N')
            || try_cmd_1args(key, "map_d", map_scroll, 'S')
            || try_cmd_1args(key, "map_r", map_scroll, 'E')
            || try_cmd_1args(key, "map_l", map_scroll, 'W')
            || try_cmd_2args(key, "g_keys_u", wrap_mv_kb_mod, 'G', 'u')
            || try_cmd_2args(key, "g_keys_d", wrap_mv_kb_mod, 'G', 'd')
            || try_cmd_2args(key, "wg_keys_u", wrap_mv_kb_mod, 'g', 'u')
            || try_cmd_2args(key, "wg_keys_d", wrap_mv_kb_mod, 'g', 'd')
            || try_cmd_2args(key, "wk_keys_u", wrap_mv_kb_mod, 'k', 'u')
            || try_cmd_2args(key, "wk_keys_d", wrap_mv_kb_mod, 'k', 'd')))
    {
        ;
    }
    return ret;
}
