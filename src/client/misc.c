/* src/client/misc.c */

#include "misc.h"
#include <ncurses.h> /* delwin() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include <stdlib.h> /* free(), exit() */
#include <string.h> /* memcpy(), strlen() */
#include <unistd.h> /* global optarg, getopt() */
#include "../common/err_try_fgets.h" /* reset_err_try_fgets_counter() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), textfile_sizes(),
                                  * try_fclose_unlink_rename(),
                                  */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "keybindings.h" /* free_keybindings(), read_keybindings_from_file(),
                          * write_keybindings_to_file()
                          */
#include "map_window.h" /* map_center() */
#include "windows.h" /* free_winDB(), make_v_screen_and_init_win_sizes(),
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
            world.path_interface = optarg;
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
    char * path = world.path_interface;
    char path_tmp[strlen(path) + 4 + 1];
    sprintf(path_tmp, "%s_tmp", path);
    FILE * file = try_fopen(path_tmp, "w", f_name);
    write_keybindings_to_file(file, &world.kb_global);
    write_keybindings_to_file(file, &world.kb_wingeom);
    write_keybindings_to_file(file, &world.kb_winkeys);
    write_order_wins_visible_active(file);
    uint8_t i;
    for (i = 0; i < strlen(world.winDB.ids); i++)
    {
        write_winconf_of_id_to_file(file, world.winDB.ids[i]);
    }
    try_fclose_unlink_rename(file, path_tmp, path, f_name);
}



extern void load_interface_conf()
{
    char * f_name = "load_interface_conf()";

    /* Read keybindings and WincConf DB from interface config file. */
    reset_err_try_fgets_counter();
    FILE * file = try_fopen(world.path_interface, "r", f_name);
    uint32_t linemax = textfile_sizes(file, NULL);
    char line[linemax + 1];
    read_keybindings_from_file(line, linemax, file, &world.kb_global);
    read_keybindings_from_file(line, linemax, file, &world.kb_wingeom);
    read_keybindings_from_file(line, linemax, file, &world.kb_winkeys);
    char active_tmp;
    char * order_tmp;
    read_order_wins_visible_active(line, linemax, file, &order_tmp, &active_tmp);
    while (read_winconf_from_file(line, linemax, file));
    try_fclose(file, f_name);

    /* Check that windows of all legal IDs have been initalized. The validity of
     * this test relies on read_winconf_from_file() failing on duplicates. Only
     * on success initialize the windows as visible, to enable safe cleaning up.
     */
    char * err = "Failed to initialize all expected windows.";
    exit_err(strlen(world.winDB.legal_ids) != strlen(world.winDB.ids), err);
    world.winDB.active = active_tmp;
    world.winDB.order = order_tmp;

    /* Build windows as defined by read interface data and toggle them on. */
    make_v_screen_and_init_win_sizes();
    char tmp_active = world.winDB.active;
    char tmp_order[strlen(world.winDB.order) + 1];
    sprintf(tmp_order, "%s", world.winDB.order);
    world.winDB.order[0] = '\0';
    uint8_t i;
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
    world.winDB.v_screen_offset = 0;
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



extern void array_append(uint32_t old_n, size_t region_size, void * new_region,
                        void ** ptr_old_array)
{
    char * f_name = "array_append()";
    uint32_t old_size = old_n * region_size;
    uint32_t new_size = old_size + region_size;
    void * new_array = try_malloc(new_size, f_name);
    memcpy(new_array, * ptr_old_array, old_size);
    memcpy(new_array + (old_n * region_size), new_region, region_size);
    free(* ptr_old_array);
    * ptr_old_array = new_array;
}
