/* misc.h
 *
 * Miscellaneous routines that have not yet found a proper parent module. Having
 * LOTS of stuff in here is a sure sign that better modularization is in order.
 */

#ifndef MISC_H
#define MISC_H

#include <stdlib.h>    /* for size_t */
#include <stdint.h>    /* for uint16_t */
#include "yx_uint16.h" /* for yx_uint16 coordinates */
struct Map;



/* Return 16-bit number pseudo-randomly generated via Linear Congruential
 * Generator algorithm with some proven constants. Use instead of rand() to
 * ensure portability of the same pseudo-randomness across systems.
 */
extern uint16_t rrand();

/* Wrappers to malloc(), calloc() from function called "f" calling exit_err()
 * with trouble_msg() error message if necessary.
 */
extern void * try_malloc(size_t size, char * f);
extern void * try_calloc(size_t nmemb, size_t size, char * f);

/* Check if tempfile "path" exists, and if so, exit with explanation that. */
extern void check_tempfile(char * path);

/* If one and only one of files at "p1", "p2" exists, fail with explanation. */
extern void check_files_xor(char * p1, char * p2);

/* Save / load / unload (free) / reload interface configuration data. */
extern void save_interface_conf();
extern void load_interface_conf();
extern void unload_interface_conf();
extern void reload_interface_conf();

/* Update game log by appending "text", or by appending a "." if "text" is the
 * same as the last one passed.
 */
extern void update_log(char * text);

/* Return the offset necessary to center a map of "mapsize" on position "pos" in
 * a frame of "framesize.
 */
extern uint16_t center_offset(uint16_t pos,
                              uint16_t mapsize, uint16_t framesize);

/* Record last player "action" in game record file "record, increment the game
 * turn and trigger enemy movement.
 */
extern void turn_over(char action);

/* Save or load current game data to / from file "savefile". */
extern void save_game();
extern void load_game();

/* Return a random position on the map "map" that is passable (as determined by
 * is_passable().
 */
extern struct yx_uint16 find_passable_pos(struct Map * map);

/* Navigate (as far as possible) up (dir=='u') or (else) down in player's
 * inventory selection.
 */
extern void nav_inventory(char dir);



#endif
