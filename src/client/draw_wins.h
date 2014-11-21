/* src/client/draw_wins.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
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
extern void draw_win_terrain_stack(struct Win * win);
extern void draw_win_keybindings_global(struct Win * win);
extern void draw_win_keybindings_winconf_geometry(struct Win * win);
extern void draw_win_keybindings_winconf_keybindings(struct Win * win);

/* Routines to draw windows' configuration views. */
extern void draw_winconf_keybindings(struct Win * win);
extern void draw_winconf_geometry(struct Win * win);



#endif
