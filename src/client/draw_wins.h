/* src/client/draw_wins.h
 *
 * Routines for drawing the game's windows' contents.
 */

#ifndef DRAW_WINS_H
#define DRAW_WINS_H

struct Win;



/* Default routines to draw the various windows' standard contents. */
extern void draw_win_log(struct Win * win);
extern void draw_win_map(struct Win * win);
extern void draw_win_info(struct Win * win);
extern void draw_win_inventory(struct Win * win);
extern void draw_win_available_keybindings(struct Win * win);
extern void draw_win_keybindings_global(struct Win * win);
extern void draw_win_keybindings_winconf_geometry(struct Win * win);
extern void draw_win_keybindings_winconf_keybindings(struct Win * win);

/* Routines to draw windows' configuration views. */
extern void draw_winconf_keybindings(struct Win * win);
extern void draw_winconf_geometry(struct Win * win);



#endif
