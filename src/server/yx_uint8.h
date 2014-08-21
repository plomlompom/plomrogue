/* src/server/yx_uint8.h
 *
 * Routines for comparison and movement with yx_uint8 structs.
 */

#ifndef YX_UINT8_H_SERVER
#define YX_UINT8_H_SERVER

#include <stdint.h> /* uint8_t */
#include "../common/yx_uint8.h" /* yx_uint8 */



/* Move "yx" into hex direction "d". If this moves "yx" beyond the minimal (0)
 * or maximal (UINT8_MAX) column or row, it wraps to the opposite side. Such
 * wrapping is returned as a wraps enum value and stored, so that further calls
 * to move "yx" back into the opposite direction may unwrap it again. Pass an
 * "unwrap" of !0 to re-set the internal wrap memory to 0.
 * Hex direction values for "d": 'e' (north-east), 'd' (east), 'c' (south-east),
 * 'x' (south-west), 's' (west), 'w' (north-west)
 */
extern uint8_t mv_yx_in_dir_wrap(char d, struct yx_uint8 * yx, uint8_t unwrap);



#endif
