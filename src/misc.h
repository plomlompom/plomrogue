#ifndef MISC_H
#define MISC_H

#include <stdint.h>
#include "yx_uint16.h"

struct World;
struct WinMeta;
struct Win;
struct Map;

extern uint16_t rrand(char, uint32_t);
extern void update_log (struct World *, char *);
extern uint16_t center_offset (uint16_t, uint16_t, uint16_t);
extern void turn_over (struct World *, char);
extern void save_game(struct World *);
extern void toggle_window (struct WinMeta *, struct Win *);
extern void scroll_pad (struct WinMeta *, char);
extern void growshrink_active_window (struct WinMeta *, char);
extern struct yx_uint16 find_passable_pos (struct Map *);
extern unsigned char meta_keys(int, struct World *, struct WinMeta *, struct Win *, struct Win *, struct Win *, struct Win *);

#endif
