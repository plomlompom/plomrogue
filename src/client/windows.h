/* src/client/windows.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * A tiled window manager for the terminal.
 *
 * It provides a virtual screen that can be scrolled horizontally and may carry
 * any number of windows to be appeared, disappeared, resized and moved around.
 * They have borders and a title bar and are positioned automatically.
 *
 * Windows can be any width between 1 and 2^16 cells. The virtual screen grows
 * with them as needed -- but only horizontally and only up to 2^16 cells. Their
 * height is limited by the height of the terminal screen (maximum 2^16 cells).
 *
 * Windows' positioning can be influenced only indirectly: by resizing them, and
 * by shifting their relative position inside the (currently invisible) chain
 * that the window manager treats their plurality as. The first window goes into
 * the top left corner of the virtual screen. Further windows are fitted as
 * left-aligned as possible below their (chain-wise) closest predecessor that
 * thrones over enough space to contain them and that is open to the right. If
 * that fails, they're fitted up-right to the window with the rightmost border.
 *
 * TODO: Think up a more intuitive window positioning algorithm.
 */

#ifndef WINDOWS_H
#define WINDOWS_H

#include <ncurses.h> /* WINDOW, chtype */
#include <stdint.h> /* uint8_t, int16_t, uint16_t, uint32_t */
#include "keybindings.h" /* struct KeyBindingDB */



struct yx_uint16
{
    uint16_t y;
    uint16_t x;
};

struct WinDB
{
    WINDOW * t_screen; /* ncurses' pointer to the terminal screen */
    WINDOW * v_screen; /* virtual screen (ncurses pad) */
    struct Win * wins; /* array of windows */
    struct yx_uint16 v_screen_size; /* virtual screen size */
    char * legal_ids; /* ids allowed to be used */
    char * ids; /* all windows' ids, followed by \0 */
    char * order; /* visible windows' id's order, followed by \0 */
    uint16_t v_screen_offset; /* how many cells v_screen view moved rightwards*/
    char active; /* id of window selected as active */
};

struct Win
{
    struct KeyBindingDB kb; /* window-specific keybindings */
    char * title; /* title to be used in window title bar */
    struct yx_uint16 target_center; /* saves .center when toggling .view */
    struct yx_uint16 frame_size; /* size of window/frame to see winmap through*/
    struct yx_uint16 start; /* upper left corner of window in v_screen */
    struct yx_uint16 center; /* winnap cell to center frame on if < winmap */
    struct yx_uint16 winmap_size; /* delimits .winmap, sorts it into lines */
    chtype * winmap; /* window content in sequence of chtype's to write */
    int16_t target_height; /* window size / .frame_size description in config */
    int16_t target_width;  /* file format, i.e. values <= 0 may be used      */
    char id; /* Win identifier; also maps to default window drawing function. */
    uint8_t target_height_type; /* 0: read .height/.width as positive size; */
    uint8_t target_width_type;  /* 1: as negative diff to v_screen size     */
    uint8_t linebreak; /* linebreaking modes: 0: wide; 1: long; 2: compact */
    uint8_t view; /* window view mode: 0: use .id- set default draw function */
};                /* 1/2: use one of the two config view draw function */



/* Get position of id "c" in world.winDB.order*/
extern uint8_t get_win_pos_in_order(char c);

/* Get Win after window identified by "c" or NULL if there is none. */
extern struct Win * get_win_after(char c);

/* Return yx offset to focus map of "mapsize" on "position" in "frame_size". */
extern uint16_t center_offset(uint16_t position,
                              uint32_t mapsize, uint32_t frame_size);

/* Get Win of "id". */
extern struct Win * get_win_by_id(char id);

/* Builds virtual sreen from .t_screen's size, fits win's sizes to them.*/
extern void make_v_screen_and_init_win_sizes();

/* Free all winDB data. */
extern void free_winDB();

/* The SIGWINCH handler winch_called() merely sets world.winch to 1. This info
 * is used by io_loop() to call reset_windows_on_winch(), which adapts the
 * currently loaded interface configuration to the new .t_screen size.
 */
extern void winch_called();
extern void reset_windows_on_winch();

/* Draw .v_screen and its windows. Add scroll hints where edges of .t_screen hit
 * .non-edges inside the virtual screen. Then update .t_screen.
 */
extern void draw_all_wins();



#endif
