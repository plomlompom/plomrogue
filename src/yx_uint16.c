#include "stdint.h"
#include "yx_uint16.h"

extern char yx_uint16_cmp (struct yx_uint16 a, struct yx_uint16 b) {
// Compare two coordinates of type yx_uint16.
  if (a.y == b.y && a.x == b.x) return 1;
  else                          return 0; }

