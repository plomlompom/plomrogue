#ifndef YX_UINT16_H
#define YX_UINT16_H

#define NORTH 1
#define EAST 2
#define SOUTH 3
#define WEST 4

struct yx_uint16 {
  uint16_t y;
  uint16_t x; };

extern char yx_uint16_cmp (struct yx_uint16, struct yx_uint16);
struct yx_uint16 mv_yx_in_dir (char, struct yx_uint16);

#endif
