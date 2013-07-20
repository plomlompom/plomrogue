#ifndef YX_UINT16_H
#define YX_UINT16_H

#include <stdint.h>

enum dir {
  NORTH = 1,
  EAST  = 2,
  SOUTH = 3,
  WEST  = 4 };

struct yx_uint16 {
  uint16_t y;
  uint16_t x; };

extern char yx_uint16_cmp (struct yx_uint16, struct yx_uint16);
extern struct yx_uint16 mv_yx_in_dir (enum dir, struct yx_uint16);

#endif
