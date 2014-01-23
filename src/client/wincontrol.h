/* src/client/wincontrol.h
 *
 * Routines that build on top of the windows library to provide a simple window
 * management API to the game. Also helps managing window-specific keybindings.
 */

#ifndef WINCONTROL_H
#define WINCONTROL_H

#include <stdint.h> /* uint8_t, int16_t */
#include "keybindings.h" /* struct KeyBindingDB */
#include "../common/yx_uint16.h" /* yx_uint16 struct */
struct Win;



struct WinConfDB
{
    struct WinConf * winconfs;
    char * ids;
    char * order; /* order of visible windows (identified by IDs) */
    char active; /* id of window selected as active */
};

/* Window's configuration (like geometry, keybindings) and the Win itself. */
struct WinConf
{
    struct Win * win;    /* Window / Win struct configured by this WinConf. */
    struct KeyBindingDB kb; /* Window-specific keybindings. */
    struct yx_uint16 center; /* Designated Win.center */
    int16_t height;      /* Designated height to pass to init_win(). */
    int16_t width;       /* Designated width to pass to init_win(). */
    uint8_t height_type; /* 0: read .height/.width as size in positive cells; */
    uint8_t width_type;  /* 1: as negative diff in cells to the screen size.  */
    uint8_t view; /* 0: use .draw as Win.draw; 1/2: use draw_winconf()_(1/2). */
    char id; /* Identifier of WinConf, also identifies Win.draw function. */
    char * title; /* Designated title to pass to init_win(). */
};



/* Get WinConf fathering "win" / get Win of WinConf of "id". */
extern struct WinConf * get_winconf_by_win(struct Win * win);
extern struct Win * get_win_by_id(char id);

/* Free all WinConf DB data. */
extern void free_winconfs();

/* Initialize  Win structs for WinConfs in WinConf database. */
extern void init_wins();

/* Toggle windows in order set by world.win_order. Point active window selection
 * to window identified by world.winconf_db.active.
 */
extern void sorted_win_toggle_and_activate();

/* Read/write world.win_order and world.winconf_db.active from/to "file". */
extern void read_order_wins_visible_active(char * line, uint32_t linemax, FILE * file);
extern void write_order_wins_visible_active(FILE * file, char * delim);

/* Iterate over chars of world.winconf_db.winconf_ids array. Restart after \0.*/
extern char get_next_winconf_id();

/* Read/write individual WinConf (identified by "c") from/to file. */
extern uint8_t read_winconf_from_file(char * line, uint32_t linemax, FILE * file);
extern void write_winconf_of_id_to_file(FILE * file, char c, char * delim);

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
