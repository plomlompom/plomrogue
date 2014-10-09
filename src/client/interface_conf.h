/* src/client/interface_conf.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Read/unread/write interface configuration file.
 */

#ifndef INTERFACE_CONF_H
#define INTERFACE_CONF_H



/* Parses command line argument -i into client configuration. */
extern void obey_argv(int argc, char * argv[]);

/* Save / load (init) / unload (free/dissolve) / reload interface configuration
 * data, world.wins.pad (initialized before opening any windows to the height of
 * the terminal screen and a width of 1) and window chains.
 *
 * Note that reload_interface_conf() also calls map_center() and re-sets
 * world.winDB.v_screen_offset to zero.
 */
extern void save_interface_conf();
extern void load_interface_conf();
extern void unload_interface_conf();
extern void reload_interface_conf();



#endif
