/* src/server/yx_uint8.h
 *
 * Routines for comparison and movement with yx_uint8 structs.
 */

#ifndef YX_UINT8_H_SERVER
#define YX_UINT8_H_SERVER

#include <stdint.h> /* uint8_t */
#include "../common/yx_uint8.h" /* yx_uint8 struct */



/* Return 1 if two yx_uint8 coordinates at "a" and "b" are equal, else 0. */
extern uint8_t yx_uint8_cmp(struct yx_uint8 * a, struct yx_uint8 * b);

/* Return yx_uint8 coordinate one step from "yx" in direction "dir" ('e':
 * northeast, 'd': east, 'c': south-east, 'x': south-west, 's': west, ' 'w':
 * north-west). If "dir" is invalid or would wrap the move around the edge of a
 * 2^8x2^8 cells field, "yx" remains unchanged.
 */
extern struct yx_uint8 mv_yx_in_dir(char dir, struct yx_uint8 yx);



#endif
