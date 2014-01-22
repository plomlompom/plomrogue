/* src/client/misc.c */

#include "misc.h"
#include <ncurses.h> /* delwin(), endwin(), refresh() */
#include <stdint.h> /* uint8_t, uint16_t */
#include <string.h> /* memset(), strlen() */
#include "cleanup.h" /* for set_cleanup_flag() */
#include "keybindings.h" /* init_keybindings(), free_keybindings(),
                          * save_keybindings()
                          */
#include "map_window.h" /* for map_center() */
#include "wincontrol.h" /* struct WinConf, init_winconfs(), init_wins(),
                         * sorted_wintoggle_and_activate(), get_win_by_id(),
                         * get_winconf_by_win(), toggle_window(),
                         * get_next_winconf_id()
                         */
#include "windows.h" /* struct Win, make_pad(), suspend_win(), free_win() */
#include "world.h" /* global world */



extern void save_interface_conf()
{
    save_keybindings("confclient/keybindings_global", &world.kb_global);
    save_keybindings("confclient/keybindings_wingeom", &world.kb_wingeom);
    save_keybindings("confclient/keybindings_winkeys", &world.kb_winkeys);
    save_win_configs();
}



extern void load_interface_conf()
{
    init_keybindings("confclient/keybindings_global",  &world.kb_global);
    init_keybindings("confclient/keybindings_wingeom", &world.kb_wingeom);
    init_keybindings("confclient/keybindings_winkeys", &world.kb_winkeys);
    init_winconfs();
    make_pad();
    init_wins();
    sorted_wintoggle_and_activate();
    set_cleanup_flag(CLEANUP_INTERFACE);
}



extern void unload_interface_conf()
{
    free_keybindings(world.kb_global.kbs);
    free_keybindings(world.kb_wingeom.kbs);
    free_keybindings(world.kb_winkeys.kbs);
    while (0 != world.wmeta.active)
    {
        suspend_win(world.wmeta.active);
    }
    free_winconfs();
    delwin(world.wmeta.pad);
}



extern void winch_called(int signal)
{
    world.winch = 1;
}



extern void reset_windows()
{
    endwin();  /* "[S]tandard way" to recalibrate ncurses post SIGWINCH, says */
    refresh(); /* <http://invisible-island.net/ncurses/ncurses-intro.html>.   */
    struct Win * w_p = world.wmeta.chain_start;
    char win_ids[strlen(world.winconf_db.winconf_ids) + 1];
    memset(win_ids, '\0', strlen(world.winconf_db.winconf_ids) + 1);
    uint8_t i = 0;
    char active = '\0';
    for (; NULL != w_p; w_p = w_p->next, i++)
    {
        struct WinConf * wc_p = get_winconf_by_win(w_p);
        win_ids[i] = wc_p->id;
        if (w_p == world.wmeta.active)
        {
            active = wc_p->id;
        }
    }
    while (0 != world.wmeta.active)
    {
        w_p = world.wmeta.active;
        suspend_win(w_p);
    }
    char id;
    while (0 != (id = get_next_winconf_id()))
    {
        free_win(get_win_by_id(id));
    }
    delwin(world.wmeta.pad);
    make_pad();
    init_wins();
    if (strlen(win_ids) < 1)
    {
        return;
    }
    for (i = 0; i < strlen(win_ids); i++)
    {
        toggle_window(win_ids[i]);
        if (active == win_ids[i])
        {
            world.wmeta.active = get_win_by_id(win_ids[i]);
        }
    }
}



extern void reload_interface_conf()
{
    unload_interface_conf();
    load_interface_conf();
    map_center();
}



extern uint16_t center_offset(uint16_t position, uint16_t mapsize,
                              uint16_t framesize)
{
    uint16_t offset = 0;
    if (mapsize > framesize)
    {
        if (position > framesize / 2)
        {
            if (position < mapsize - (framesize / 2))
            {
                offset = position - (framesize / 2);
            }
            else
            {
                offset = mapsize - framesize;
            }
        }
    }
    return offset;
}



extern void nav_inventory(char dir)
{
    if ('u' == dir)
    {
        world.player_inventory_select = world.player_inventory_select
                                        - (world.player_inventory_select > 0);
        return;
    }
    uint8_t n_elems = 0;
    uint8_t i;
    for (i = 0; '\0' != world.player_inventory[i]; i++)
    {
        n_elems = n_elems + ('\n' == world.player_inventory[i]);
    }
    world.player_inventory_select = world.player_inventory_select
                                    + (world.player_inventory_select < n_elems);
}

