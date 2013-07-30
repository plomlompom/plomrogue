/* draw_wins.h
 *
 * Routnes for drawing the game's windows' contents.
 */

#ifndef DRAW_WINS_H
#define DRAW_WINS_H



#include <stdint.h> /* for uint16_t */
struct Win;



/* Write game log text into "win" from bottom to top. */
extern void draw_log_win(struct Win * win);

/* Draw game map and actors/objects on it into "win". Respect scroll offset. */
extern void draw_map_win(struct Win * win);

/* Draw into "win" the game / player status infos. */
extern void draw_info_win(struct Win * win);

/* Draw keybindings selection/manipulation menu. */
extern void draw_keys_win(struct Win * win);



#endif
