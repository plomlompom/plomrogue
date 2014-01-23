/* src/client/misc.h
 *
 * Miscellaneous routines that have not yet found a proper parent module. Having
 * LOTS of stuff in here is a sure sign that better modularization is in order.
 */

#ifndef MISC_H
#define MISC_H

#include <stdint.h> /* for uint16_t */



/* Parses command line argument -i into client configuration. */
extern void obey_argv(int argc, char * argv[]);

/* Save / load (init) / unload (free/dissolve) / reload interface configuration
 * data, world.wins.pad (initialized before opening any windows to the height of
 * the terminal screen and a width of 1) and window chains.
 *
 * Note that reload_interface_conf() also calls map_center().
 */
extern void save_interface_conf();
extern void load_interface_conf();
extern void unload_interface_conf();
extern void reload_interface_conf();

/* The SIGWINCH handler winch_called() merely sets world.winch to 1. This info
 * is used by io_loop() to call reset_windows(), which adapts the currently
 * loaded interface configuration to the new screen size.
 */
extern void winch_called();
extern void reset_windows();

/* Return offset into center map of "mapsize" on "position" in "framesize". */
extern uint16_t center_offset(uint16_t position,
                              uint16_t mapsize, uint16_t framesize);

/* Move world.inventory_sel up ("dir"="u") or down (else) as far as possible. */
extern void nav_inventory(char dir);



#endif
