/* src/client/misc.c */

#include "misc.h"
#include <stdlib.h> /* exit() */
#include <ncurses.h> /* delwin() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* sprintf() */
#include <string.h> /* strlen() */
#include <unistd.h> /* global optarg, getopt() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), textfile_sizes(),
                                  * try_fclose_unlink_rename(),
                                  */
#include "cleanup.h" /* set_cleanup_flag() */
#include "keybindings.h" /* free_keybindings(), read_keybindings_from_file(),
                          * write_keybindings_to_file()
                          */
#include "map_window.h" /* map_center() */
#include "windows.h" /* for free_winDB(), make_v_screen_and_init_win_sizes(),
                      * read_winconf_from_file(), write_winconf_of_id_to_file(),
                      * toggle_window()
                      */
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
    for (i = 0; i < strlen(world.winDB.ids); i++)
    {
        write_winconf_of_id_to_file(file, world.winDB.ids[i], delim);
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
    make_v_screen_and_init_win_sizes();
    uint8_t i;
    char tmp_active = world.winDB.active;
    char tmp_order[strlen(world.winDB.order) + 1];
    sprintf(tmp_order, "%s", world.winDB.order);
    world.winDB.order[0] = '\0';
    for (i = 0; i < strlen(tmp_order); toggle_window(tmp_order[i]), i++);
    world.winDB.active = tmp_active;

    /* So that the interface config data and the window structs get freed. */
    set_cleanup_flag(CLEANUP_INTERFACE);
}



extern void unload_interface_conf()
{
    free_keybindings(world.kb_global.kbs);
    free_keybindings(world.kb_wingeom.kbs);
    free_keybindings(world.kb_winkeys.kbs);
    while ('\0' != world.winDB.active)
    {
        toggle_window(world.winDB.active);
    }
    free_winDB();
    delwin(world.winDB.v_screen);
}



extern void reload_interface_conf()
{
    unload_interface_conf();
    load_interface_conf();
    map_center();
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
