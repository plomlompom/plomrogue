/* src/server/yx_uint16.h
 *
 * Routines for comparison and movement with yx_uin16 structs.
 */

#ifndef YX_UINT16_H_SERVER
#define YX_UINT16_H_SERVER

#include <stdint.h> /* uint8_t */
#include "../common/yx_uint16.h" /* yx_uint16 struct */



/* Return 1 if two yx_uint16 coordinates at "a" and "b" are equal, else 0. */
extern uint8_t yx_uint16_cmp(struct yx_uint16 * a, struct yx_uint16 * b);

/* Return yx_uint16 coordinate one step from "yx" in direction "dir" (numpad
 * digits: north '8', east: '6', etc.) If "dir" is invalid or would wrap the
 * move around the edge of a 2^16x2^16 cells field, "yx" remains unchanged.
 */
extern struct yx_uint16 mv_yx_in_dir(char dir, struct yx_uint16 yx);



#endif
