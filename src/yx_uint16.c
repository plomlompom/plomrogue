#include "yx_uint16.h"

extern char yx_uint16_cmp (struct yx_uint16 a, struct yx_uint16 b) {
// Compare two coordinates of type yx_uint16.
  if (a.y == b.y && a.x == b.x) return 1;
  else                          return 0; }

extern struct yx_uint16 mv_yx_in_dir (enum dir d, struct yx_uint16 yx) {
// Return yx coordinates one step to the direction d of yx.
  if      (d == NORTH) yx.y--;
  else if (d == EAST)  yx.x++;
  else if (d == SOUTH) yx.y++;
  else if (d == WEST)  yx.x--;
  return yx; }

