#ifndef DRAW_WINS_H
#define DRAW_WINS_H

void draw_with_linebreaks (struct Win *, char *, uint16_t);
void draw_text_from_bottom (struct Win *, char *);
void draw_log_win (struct Win *);
void draw_map_win (struct Win *);
void draw_info_win (struct Win *);
void draw_keys_win (struct Win *);

#endif
