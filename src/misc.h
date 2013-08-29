/* misc.h
 *
 * Miscellaneous routines that have not yet found a proper parent module. Having
 * LOTS of stuff in here is a sure sign that better modularization is in order.
 */

#ifndef MISC_H
#define MISC_H



#include <stdint.h>    /* for uint16_t */
#include <stdio.h>     /* for FILE typedef */
#include "yx_uint16.h" /* for yx_uint16 coordinates */
struct World;
struct Map;



/* Learn from "file" the largest line length (pointed to by "linemax_p") and
 * (pointed to by "n_lines_p" if it is not set to NULL) the number of lines.
 */
extern void textfile_sizes(FILE * file, uint16_t * linemax_p,
                           uint16_t * n_lines_p);



/* Update game log by appending "text", or by appending a "." if "text" is the
 * same as the last one passed.
 */
extern void update_log(struct World * world, char * text);



/* Return the offset necessary to center "map" on position "pos" in a frame of
 * "framesize.
 */
extern uint16_t center_offset(uint16_t pos, uint16_t mapsize,
                              uint16_t framesize);



/* Record last player "action" in game record file "record, increment the game
 * turn and trigger enemy movement.
 */
extern void turn_over(struct World * world, char action);



/* Save current game data to file "savefile". */
extern void save_game(struct World * world);



/* Return a random position on the map "map" that is passable (as determined by
 * is_passable().
 */
extern struct yx_uint16 find_passable_pos(struct Map * map);



#endif
