/* src/client/misc.c */

#include "misc.h"
#include <stdint.h> /* uint8_t, uint16_t */
#include "cleanup.h" /* for set_cleanup_flag() */
#include "keybindings.h" /* init_keybindings(), free_keybindings(),
                          * save_keybindings()
                          */
#include "wincontrol.h" /* init_winconfs(), init_wins(),
                         * sorted_wintoggle_and_activate()
                         */
#include "windows.h" /* suspend_win() */
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
}



extern void reload_interface_conf()
{
    unload_interface_conf();
    load_interface_conf();
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

