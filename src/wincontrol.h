/* wincontrol.h
 *
 * Routines that build on top of the windows library to provide a simple window
 * management API to the game. Also helps managing window-specific keybindings.
 */

#ifndef WINCONTROL_H
#define WINCONTROL_H

#include <stdint.h> /* for uint8_t, int16_t */
#include "keybindings.h" /* for KeyBiData struct */
#include "yx_uint16.h" /* for yx_uint16 struct */
struct Win;



/* Stores a window's configuration (like geometry, keybindings) and a pointer to
 * the respective Win struct itself.
 */
struct WinConf
{
    char id; /* Unique identifier of WinConf, doubles aas identifier for .win */
             /* and the char following "Win_" in the respective conffile name.*/
    struct Win * win;    /* Window / Win struct configured by this WinConf. */
    struct KeyBiData kb; /* Window-specific keybindings. */
    uint8_t view; /* 0: use .draw as Win.draw; 1/2: use draw_winconf()_(1/2). */
    int16_t height;      /* Designated height to pass to init_win(). */
    int16_t width;       /* Designated width to pass to init_win(). */
    uint8_t height_type; /* 0: read .height/.width as size in positive cells; */
    uint8_t width_type;  /* 1: as negative diff in cells to the screen size.  */
    char * title; /* Designated title to pass to init_win(). */
    char draw;    /* Identifier of designated Win.draw; passed to init_win() */
                  /* and reset after toggling Win.draw via toggle_winconf(). */
    struct yx_uint16 center; /* Designated Win.center; to be reset after  */
};                           /* toggling Win.center via toggle_winconf(). */



/* Get WinConf fathering "win" / get Win of WinConf of "id". */
extern struct WinConf * get_winconf_by_win(struct Win * win);
extern struct Win * get_win_by_id(char id);

/* Create, initialize (from config files)/free world.winconfs and their Wins. */
extern void init_winconfs();
extern void free_winconfs();
extern void init_wins();

/* Toggle windows in the order desribed by the 1st line of
 * config/windows/toggle_order_and_active, where each char may fit a Winconf.id
 * in world.winconfs. Silently ignore id chars not found there. The 1st char of
 * the 2nd line of the same file determines which window (by its .id) to focus
 * as active (but only if this window belongs to the ones just toggled).
 */
extern void sorted_wintoggle_and_activate();

/* Save world.winconfs, visible window chain and active window selection to the
 * respective configuration files in config/windows/.
 */
extern void save_win_configs();

/* Toggle "window configuration" view for active window. Sets sensible
 * Win.center values for the various configuration views (for winconf_geometry:
 * y=0, x=0; for winconf_keys: x=0 (y is set by draw_winconf_keybindings()).
 */
extern void toggle_winconfig();

/* Toggle WinConf.(height/width)_type ("axis" = "y": height; else: width). Avoid
 * positive diff to screen width (value would be wrongly read as a non-diff),
 * width_type toggles to 1 only if world.wmeta->screen's width >= WinConf.width.
 */
extern void toggle_win_size_type(char axis);

/* Toggle display of a window identified by "id". */
extern void toggle_window(char id);

/* Try scrolling virtual screen left ("dir" = "-") or right ("dir" = "+") to the
 * degree allowed by the window manager's reset_pad_offset().
 */
extern void scroll_pad(char dir);

/* Try to grow or shrink the active window horizontally ("change" = "*"/"_") or
 * vertically ("change = "+"/"-") by one cell size to the degree allowed by the
 * window manager's resize_active_win(). If a new window width would surpass
 * that of the terminal screen, set WinConf.width_type to 0.
 */
extern void growshrink_active_window(char change);



#endif
