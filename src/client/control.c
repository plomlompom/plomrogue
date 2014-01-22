/* src/client/control.c */

#include "control.h"
#include <stdint.h> /* uint8_t, uint16_t */
#include <stdio.h> /* sprintf() */
#include <string.h> /* strlen() */
#include "io.h" /* try_send() */
#include "keybindings.h" /* get_command_to_keycode(), get_keycode_to_command(),
                          * mod_selected_keyb(), move_keyb_selection()
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



/* If "command"'s .dsc_short fits "match", apply "f" with provided char
 * arguments and return 1; else, return 0.
 */
static uint8_t try_0args(struct Command * command, char * match, void (* f) ());
static uint8_t try_1args(struct Command * command, char * match,
                         void (* f) (char), char c);
static uint8_t try_2args(struct Command * command, char * match,
                         void (* f) (char, char), char c1, char c2);

/* Try if "command" matches a hard-coded list of client-only commands and, if
 * successful, execute the match and return 1. Else, return 0.
 */
static uint8_t try_client_commands(struct Command * command);

/* Try out "command" as one for server messaging; sending is .server_msg,
 * followed by either a string representing "command"'s .arg, or, if .arg is
 * 'i', world.player_inventory_select. Return 1 on success, 0 on failure.
 */
static uint8_t try_server_commands(struct Command * command);



static uint8_t try_0args(struct Command * command, char * match, void (* f) ())
{
    if (!strcmp(command->dsc_short, match))
    {
        f();
        return 1;
    }
    return 0;
}

static uint8_t try_1args(struct Command * command, char * match,
                             void (* f) (char), char c)
{
    if (!strcmp(command->dsc_short, match))
    {
        f(c);
        return 1;
    }
    return 0;
}



static uint8_t try_2args(struct Command * command, char * match,
                             void (* f) (char, char), char c1, char c2)
{
    if (!strcmp(command->dsc_short, match))
    {
        f(c1, c2);
        return 1;
    }
    return 0;
}



static uint8_t try_client_commands(struct Command * command)
{
    return (   try_0args(command, "map_c", map_center)
            || try_1args(command, "map_u", map_scroll, 'N')
            || try_1args(command, "map_d", map_scroll, 'S')
            || try_1args(command, "map_r", map_scroll, 'E')
            || try_1args(command, "map_l", map_scroll, 'W')
            || try_1args(command, "inv_u", nav_inventory, 'u')
            || try_1args(command, "inv_d", nav_inventory, 'd')
            || try_1args(command, "cyc_win_f", cycle_active_win, 'f')
            || try_1args(command, "cyc_win_b", cycle_active_win, 'b')
            || try_1args(command, "scrl_r", scroll_pad, '+')
            || try_1args(command, "scrl_l", scroll_pad, '-')
            || try_1args(command, "to_a_keywin", toggle_window, 'k')
            || try_1args(command, "to_g_keywin", toggle_window, '0')
            || try_1args(command, "to_wg_keywin", toggle_window, '1')
            || try_1args(command, "to_wk_keywin", toggle_window, '2')
            || try_1args(command, "to_mapwin", toggle_window, 'm')
            || try_1args(command, "to_infowin", toggle_window, 'i')
            || try_1args(command, "to_inv", toggle_window, 'c')
            || try_1args(command, "to_logwin", toggle_window, 'l')
            || try_0args(command, "winconf", toggle_winconfig)
            || try_1args(command, "grow_h", growshrink_active_window, '*')
            || try_1args(command, "shri_h", growshrink_active_window, '_')
            || try_1args(command, "grow_v", growshrink_active_window, '+')
            || try_1args(command, "shri_v", growshrink_active_window, '-')
            || try_1args(command, "to_height_t", toggle_win_size_type, 'y')
            || try_1args(command, "to_width_t", toggle_win_size_type, 'x')
            || try_1args(command, "shift_f", shift_active_win, 'f')
            || try_1args(command, "shift_b", shift_active_win, 'b')
            || try_0args(command, "reload_conf", reload_interface_conf)
            || try_0args(command, "save_conf", save_interface_conf)
            || try_1args(command, "g_keys_m", mod_selected_keyb, 'G')
            || try_2args(command, "g_keys_u", move_keyb_selection, 'G', 'u')
            || try_2args(command, "g_keys_d", move_keyb_selection, 'G', 'd')
            || try_1args(command, "w_keys_m", mod_selected_keyb, 'w')
            || try_2args(command, "w_keys_u", move_keyb_selection, 'w', 'u')
            || try_2args(command, "w_keys_d", move_keyb_selection, 'w', 'd')
            || try_1args(command, "wg_keys_m", mod_selected_keyb, 'g')
            || try_2args(command, "wg_keys_u", move_keyb_selection, 'g', 'u')
            || try_2args(command, "wg_keys_d", move_keyb_selection, 'g', 'd')
            || try_1args(command, "wk_keys_m", mod_selected_keyb, 'k')
            || try_2args(command, "wk_keys_u", move_keyb_selection, 'k', 'u')
            || try_2args(command, "wk_keys_d", move_keyb_selection, 'k', 'd'));
}



static uint8_t try_server_commands(struct Command * command)
{
    if (command->server_msg)
    {
        uint8_t arg = (uint8_t) command->arg;
        if ('i' == arg)
        {
            arg = world.player_inventory_select;
        }
        uint8_t command_size = strlen(command->server_msg);
        uint8_t arg_size = 3;
        char msg[command_size + 1 + arg_size + 1];
        sprintf(msg, "%s %d", command->server_msg, arg);
        try_send(msg);
        return 1;
    }
    return 0;
}



extern uint8_t try_key(uint16_t key)
{
    struct Command * command = get_command_to_keycode(world.kb_global.kbs, key);
    if (!command)
    {
        struct WinConf * wc = get_winconf_by_win(world.wmeta.active);
        if      (0 == wc->view)
        {
            command = get_command_to_keycode(wc->kb.kbs, key);
        }
        else if (1 == wc->view)
        {
            command = get_command_to_keycode(world.kb_wingeom.kbs, key);
        }
        else if (2 == wc->view)
        {
            command = get_command_to_keycode(world.kb_winkeys.kbs, key);
        }
    }
    if (command)
    {
        if (try_server_commands(command))
        {
            return 1;
        }
        else if      (try_client_commands(command))
        {
            return 1;
        }
        else if (!strcmp("quit", command->dsc_short))
        {
            return 2;
        }
    }
    return 0;
}