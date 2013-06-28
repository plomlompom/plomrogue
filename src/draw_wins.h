#ifndef DRAW_WINS_H
#define DRAW_WINS_H

#include <stdint.h>

struct Win;

extern void draw_with_linebreaks (struct Win *, char *, uint16_t);
extern void draw_text_from_bottom (struct Win *, char *);
extern void draw_log_win (struct Win *);
extern void draw_map_win (struct Win *);
extern void draw_info_win (struct Win *);
extern void draw_keys_win (struct Win *);

#endif
