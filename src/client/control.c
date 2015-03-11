/* src/client/control.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "control.h"
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT32_MAX */
#include <stdlib.h> /* free() */
#include <stdio.h> /* sprintf() */
#include <string.h> /* strlen(), strcmp(), strncmp() */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "interface_conf.h" /* reload_interface_conf(), save_interface_conf() */
#include "io.h" /* send() */
#include "keybindings.h" /* get_command_to_keycode(), get_keycode_to_command(),
                          * mod_selected_keyb(), move_keyb_selection()
                          */
#include "map.h" /* toggle_lookmode(), lookmode_nav()*/
#include "wincontrol.h" /* shift_active_win(), resize_active_win(),
                         * toggle_win_size_type(), toggle_window(),
                         * cycle_active_win(), scroll_v_screen(),
                         * toggle_linebreak_type(), toggle_winconfig()
                         */
#include "windows.h" /* get_win_by_id() */
#include "world.h" /* world */



/* Move world.inventory_sel up ("dir"="u") or down (else) as far as possible. */
static void nav_inventory(char dir);

/* If "command"'s .dsc_short fits "match", apply "f" with provided char
 * arguments and return 1; else, return 0.
 */
static uint8_t try_0args(struct Command * command, char * match, void (* f) ());
static uint8_t try_1args(struct Command * command, char * match,
                         void (* f) (char), char c);

/* If "command" fits pattern "keyb_XY" with Y a proper keybinding list ID char
 * and X one of "u" (for "up"), "d" (for "down") or "m" (for "modify"), move up
 * or down or modify entry in the selected keybinding list.
 */
static uint8_t try_kb_manip(char * command);

/* Try if "command" matches a hard-coded list of client-only commands and, if
 * successful, execute the match and return 1. Else, return 0.
 */
static uint8_t try_client_commands(struct Command * command);

/* If c == c_to_match, set "string" to "string_to_set". */
static uint8_t set_string_if_char_match(char c, char c_to_match,
                                        char ** string, char * string_to_set);

/* Transform "command" to server command + argument string (free externally). */
static char * build_server_message_with_argument(struct Command * command);

/* Try out "command" as one for server messaging; sending is .server_msg,
 * followed by either a string representing "command"'s .arg, or, if .arg is
 * 'i', world.player_inventory_select, or, if .arg is '0', nothing. Return 1 on
 * success, 0 on failure.
 */
static uint8_t try_server_commands(struct Command * command);



static void nav_inventory(char dir)
{
    if ('u' == dir)
    {
        world.player_inventory_select = world.player_inventory_select
                                        - (world.player_inventory_select > 0);
        return;
    }
    uint8_t n_elems = 0;
    uint32_t i;
    char * err = "Inventory string is too large.";
    exit_err(UINT32_MAX <= strlen(world.player_inventory), err);
    for (i = 0; '\0' != world.player_inventory[i]; i++)
    {
        n_elems = n_elems + ('\n' == world.player_inventory[i]);
    }
    world.player_inventory_select = world.player_inventory_select
                                    + (world.player_inventory_select < n_elems);
}



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



static uint8_t try_kb_manip(char * command)
{
    char * cmp = "keyb_";
    if (strlen(command) == strlen(cmp)+2 && !strncmp(command, cmp, strlen(cmp)))
    {
        if ('m' == command[strlen(cmp)])
        {
            mod_selected_keyb(command[strlen(cmp) + 1]);
        }
        else if ('u' == command[strlen(cmp)] || 'd' == command[strlen(cmp)])
        {
            move_keyb_selection(command[strlen(cmp) + 1], command[strlen(cmp)]);
        }
        return 1;
    }
    return 0;
}



static uint8_t try_client_commands(struct Command * command)
{
    return (   try_1args(command, "inv_u", nav_inventory, 'u')
            || try_1args(command, "inv_d", nav_inventory, 'd')
            || try_1args(command, "cyc_win_f", cycle_active_win, 'f')
            || try_1args(command, "cyc_win_b", cycle_active_win, 'b')
            || try_1args(command, "scrl_r", scroll_v_screen, '+')
            || try_1args(command, "scrl_l", scroll_v_screen, '-')
            || try_1args(command, "to_g_keywin", toggle_window, '0')
            || try_1args(command, "to_wg_keywin", toggle_window, '1')
            || try_1args(command, "to_wk_keywin", toggle_window, '2')
            || try_1args(command, "to_mapwin", toggle_window, 'm')
            || try_1args(command, "to_infowin", toggle_window, 'i')
            || try_1args(command, "to_inv", toggle_window, 'c')
            || try_1args(command, "to_logwin", toggle_window, 'l')
            || try_1args(command, "to_terrain", toggle_window, 's')
            || try_0args(command, "winconf", toggle_winconfig)
            || try_1args(command, "grow_h", resize_active_win, '*')
            || try_1args(command, "shri_h", resize_active_win, '_')
            || try_1args(command, "grow_v", resize_active_win, '+')
            || try_1args(command, "shri_v", resize_active_win, '-')
            || try_0args(command, "to_break", toggle_linebreak_type)
            || try_1args(command, "to_height_t", toggle_win_size_type, 'Y') //
            || try_1args(command, "to_width_t", toggle_win_size_type, 'X') //
            || try_1args(command, "shift_f", shift_active_win, 'f')
            || try_1args(command, "shift_b", shift_active_win, 'b')
            || try_0args(command, "reload_conf", reload_interface_conf)
            || try_0args(command, "save_conf", save_interface_conf)
            || try_0args(command, "to_look", toggle_lookmode)
            || try_kb_manip(command->dsc_short));
}



static uint8_t set_string_if_char_match(char c, char c_to_match,
                                        char ** string, char * string_to_set)
{
    if (c == c_to_match)
    {
        *string = string_to_set;
        return 1;
    }
    return 0;
}



static char * build_server_message_with_argument(struct Command * cmd)
{
    uint8_t command_size = strlen(cmd->server_msg);
    char * arg_str = "";
    uint8_t arg_size = 0;
    if ('i' == cmd->arg)
    {
        arg_size = 3;
        arg_str = try_malloc(arg_size + 1, __func__);
        int test = sprintf(arg_str, "%d",world.player_inventory_select);
        exit_trouble(test < 0, __func__, "sprintf");
    }
    else if (   set_string_if_char_match(cmd->arg, 'd', &arg_str, "east")
             || set_string_if_char_match(cmd->arg, 'c', &arg_str, "south-east")
             || set_string_if_char_match(cmd->arg, 'x', &arg_str, "south-west")
             || set_string_if_char_match(cmd->arg, 's', &arg_str, "west")
             || set_string_if_char_match(cmd->arg, 'w', &arg_str, "north-west")
             || set_string_if_char_match(cmd->arg, 'e', &arg_str, "north-east"))
    {
        arg_size = strlen(arg_str);
    }
    else
    {
        exit_err(1, "Illegal server command argument.");
    }
    char * msg = try_malloc(command_size + 1 + arg_size + 1, __func__);
    int test = sprintf(msg, "%s %s", cmd->server_msg, arg_str);
    exit_trouble(test < 0, __func__, "sprintf");
    if ('i' == cmd->arg)
    {
        free(arg_str);
    }
    return msg;
}



static uint8_t try_server_commands(struct Command * command)
{
    if (command->server_msg)
    {
        if ('0' == command->arg)
        {
            send(command->server_msg);
        }
        else
        {
            char * msg = build_server_message_with_argument(command);
            send(msg);
            free(msg);
        }
        return 1;
    }
    return 0;
}



extern uint8_t try_key(uint16_t key)
{
    struct Command * command = get_command_to_keycode(&world.kb_global, key);
    if (!command && world.winDB.active)
    {
        struct Win * w = get_win_by_id(world.winDB.active);
        if      (0 == w->view)
        {
            command = get_command_to_keycode(&w->kb, key);
        }
        else if (1 == w->view)
        {
            command = get_command_to_keycode(&world.kb_wingeom, key);
        }
        else if (2 == w->view)
        {
            command = get_command_to_keycode(&world.kb_winkeys, key);
        }
    }
    if (command)
    {
        if (world.look && lookmode_nav(command->dsc_short))
        {
            return 1;
        }
        else if (try_client_commands(command))
        {
            return 1;
        }
        else if (try_server_commands(command))
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
