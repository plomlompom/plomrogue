/* src/client/misc.c */

#include "misc.h"
#include <ncurses.h> /* delwin(), endwin(), refresh() */
#include <stdint.h> /* uint8_t, uint16_t */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* exit(), free() */
#include <string.h> /* memset(), strlen() */
#include <unistd.h> /* global optarg, getopt() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(),
                                  * try_fclose_unlink_rename(),
                                  */
#include "cleanup.h" /* for set_cleanup_flag() */
#include "keybindings.h" /* free_keybindings(), read_keybindings_from_file(),
                          * write_keybindings_to_file()
                          */
#include "map_window.h" /* for map_center() */
#include "wincontrol.h" /* struct WinConf, init_wins(), get_winconf_by_win(),
                         * sorted_win_toggle_and_activate(), get_win_by_id(),
                         * toggle_window(), write_winconf_of_id_to_file(),
                         * read_winconf_from_file(), get_next_winconf_id(),
                         * read_order_wins_visible_active(),
                         * write_order_wins_visible_active()
                         */
#include "windows.h" /* struct Win, make_pad(), suspend_win(), free_win() */
#include "world.h" /* global world */



extern void obey_argv(int argc, char * argv[])
{
    int opt;
    while (-1 != (opt = getopt(argc, argv, "i:")))
    {
        if      ('i' == opt)
        {
            world.path_interface_conf = optarg;
        }
        else
        {
            exit(EXIT_FAILURE);
        }
    }
}



extern void save_interface_conf()
{
    char * f_name = "save_interface_conf()";
    char * path = world.path_interface_conf;
    char path_tmp[strlen(path) + 4 + 1];
    sprintf(path_tmp, "%s_tmp", path);
    FILE * file = try_fopen(path_tmp, "w", f_name);
    char * delim = "%\n";
    write_keybindings_to_file(file, &world.kb_global, delim);
    write_keybindings_to_file(file, &world.kb_wingeom, delim);
    write_keybindings_to_file(file, &world.kb_winkeys, delim);
    write_order_wins_visible_active(file, delim);
    uint8_t i;
    for (i = 0; i < strlen(world.winconf_db.ids); i++)
    {
        write_winconf_of_id_to_file(file, world.winconf_db.ids[i], delim);
    }
    try_fclose_unlink_rename(file, path_tmp, path, f_name);
}



extern void load_interface_conf()
{
    char * f_name = "load_interface_conf()";

    /* Read keybindings and WincConf DB from interface config file. */
    FILE * file = try_fopen(world.path_interface_conf, "r", f_name);
    uint32_t linemax = textfile_sizes(file, NULL);
    char line[linemax + 1];
    read_keybindings_from_file(line, linemax, file, &world.kb_global);
    read_keybindings_from_file(line, linemax, file, &world.kb_wingeom);
    read_keybindings_from_file(line, linemax, file, &world.kb_winkeys);
    read_order_wins_visible_active(line, linemax, file);
    while (read_winconf_from_file(line, linemax, file));
    try_fclose(file, f_name);

    /* Build windows as defined by read interface data and toggle them on. */
    make_pad();
    init_wins();
    sorted_win_toggle_and_activate();

    /* So that the interface config data and the window structs get freed. */
    set_cleanup_flag(CLEANUP_INTERFACE);
}



extern void unload_interface_conf()
{
    free_keybindings(world.kb_global.kbs);
    world.kb_global.kbs = NULL;
    free_keybindings(world.kb_wingeom.kbs);
    world.kb_wingeom.kbs = NULL;
    free_keybindings(world.kb_winkeys.kbs);
    world.kb_winkeys.kbs = NULL;
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
    char win_ids[strlen(world.winconf_db.ids) + 1];
    memset(win_ids, '\0', strlen(world.winconf_db.ids) + 1);
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

