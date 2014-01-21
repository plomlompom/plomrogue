/* src/client/control.c */

#include "control.h"
#include <stdint.h> /* uint8_t, uint16_t */
#include <stdio.h> /* sprintf() */
#include <string.h> /* strlen() */
#include "command_db.h" /* get_command_id(), is_command_id_shortdsc() */
#include "io.h" /* try_send() */
#include "keybindings.h" /* struct KeyBindingDB, get_command_to_keycode(),
                          * get_keycode_to_command(), mod_selected_keyb(),
                          * move_keyb_mod_selection()
                          */
#include "map_window.h" /* for map_scroll(), map_center() */
#include "misc.h" /* reload_interface_conf(), save_interface_conf(),
                   * nav_inventory()
                   */
#include "wincontrol.h" /* struct WinConf, toggle_window(), toggle_winconfig(),
                         * scroll_pad(), get_winconf_by_win(),
                         * growshrink_active_window(), toggle_win_size_type()
                         */
#include "windows.h" /* for cycle_active_win(), shift_active_win() */
#include "world.h" /* for global world */



/* If "keycode" matches "match" in get_available_keycode_to_command(), execute
 * "f" with provided char arguments and return 1; else only return 0.
 */
static uint8_t try_cmd_0args(int keycode, char * match, void (* f) ());
static uint8_t try_cmd_1args(int keycode, char * match,
                             void (* f) (char), char c);
static uint8_t try_cmd_2args(int keycode, char * match,
                             void (* f) (char, char), char c1, char c2);

/* If "command_id" is id of command named "match", send (via try_send()) a
 * string of "match" + " " + the string representation of "arg" to the server.
 */
static uint8_t try_player_cmd(int command_id, char * match, char * command_name,
                              uint8_t arg);

/* Return keycode to "command" if it is available in current window config. */
static uint16_t get_available_keycode_to_command(char * command);

/* Return pointer to global keybindings or to keybindings for wingeometry config
 * (c = "g") or winkeys config (c = "k") or active window's keybindings ("w").
 */
static struct KeyBindingDB * select_keybindingdb_pointer(char c);

/* Wrappers to make some functions compatible to try_cmd_* single char args. */
static void wrap_mod_selected_keyb(char c);
static void wrap_mv_kb_mod(char c1, char c2);



static uint8_t try_cmd_0args(int keycode, char * match, void (* f) ())
{
    if (keycode == get_available_keycode_to_command(match))
    {
        f();
        return 1;
    }
    return 0;
}



static uint8_t try_cmd_1args(int keycode, char * match,
                             void (* f) (char), char c)
{
    if (keycode == get_available_keycode_to_command(match))
    {
        f(c);
        return 1;
    }
    return 0;
}



static uint8_t try_cmd_2args(int keycode, char * match,
                             void (* f) (char, char), char c1, char c2)
{
    if (keycode == get_available_keycode_to_command(match))
    {
        f(c1, c2);
        return 1;
    }
    return 0;
}



static uint8_t try_player_cmd(int command_id, char * match, char * command_name,
                              uint8_t arg)
{
    if (is_command_id_shortdsc(command_id, match))
    {
        uint8_t command_size = strlen(command_name);
        uint8_t arg_size = 3;
        char msg[command_size + 1 + arg_size + 1];
        sprintf(msg, "%s %d", command_name, arg);
        try_send(msg);
        return 1;
    }
    return 0;
}



static uint16_t get_available_keycode_to_command(char * command)
{
    uint16_t keycode = get_keycode_to_command(world.kb_global.kbs, command);
    if (0 != keycode || 0 == world.wmeta.active)
    {
        return keycode;
    }
    struct WinConf * wc = get_winconf_by_win(world.wmeta.active);
    if (0 == wc->view)
    {
        keycode = get_keycode_to_command(wc->kb.kbs, command);
    }
    else if (1 == wc->view)
    {
        keycode = get_keycode_to_command(world.kb_wingeom.kbs, command);
    }
    else if (2 == wc->view)
    {
        keycode = get_keycode_to_command(world.kb_winkeys.kbs, command);
    }
    return keycode;
}



static struct KeyBindingDB * select_keybindingdb_pointer(char c)
{
    struct KeyBindingDB * kbd;
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
        struct WinConf * wc = get_winconf_by_win(world.wmeta.active);
        kbd = &wc->kb;
    }
    return kbd;
}



static void wrap_mod_selected_keyb(char c)
{
        mod_selected_keyb(select_keybindingdb_pointer(c));
}



static void wrap_mv_kb_mod(char c1, char c2)
{
        move_keyb_mod_selection(select_keybindingdb_pointer(c1), c2);
}



extern uint8_t player_control(int key)
{
    char * command = get_command_to_keycode(world.kb_global.kbs, key);
    if (NULL == command && 0 != world.wmeta.active)
    {
        struct WinConf * wc = get_winconf_by_win(world.wmeta.active);
        command = get_command_to_keycode(wc->kb.kbs, key);
    }
    if (NULL != command)
    {
        uint8_t id = get_command_id(command);
        if (   try_player_cmd(id, "wait", "wait", 0)
            || try_player_cmd(id, "drop", "drop", world.player_inventory_select)
            || try_player_cmd(id, "pick", "pick_up", 0)
            || try_player_cmd(id, "use", "use", world.player_inventory_select)
            || try_player_cmd(id, "player_u", "move", 'N')
            || try_player_cmd(id, "player_d", "move", 'S')
            || try_player_cmd(id, "player_r", "move", 'E')
            || try_player_cmd(id, "player_l", "move", 'W'))
        {
            return 1;
        }
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
    uint8_t ret = 2 * (key == get_available_keycode_to_command("quit"));
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
        ret = 1;
    }
    return ret;
}
