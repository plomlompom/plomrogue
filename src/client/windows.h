/* src/client/windows.h
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
#include <stdint.h> /* uint8_t, int16_t, uint16_t */
#include "keybindings.h" /* struct KeyBindingDB */
#include "../common/yx_uint16.h" /* yx_uint16 struct */



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
    uint8_t view; /* winde view mode: 0: use default draw function set by .id */
};                /* 1/2: use one of the two config view draw function */



/* Return yx offset to focus map of "mapsize" on "position" in "frame_size". */
extern uint16_t center_offset(uint16_t position,
                              uint16_t mapsize, uint16_t frame_size);

/* Get Win of "id". */
extern struct Win * get_win_by_id(char id);

/* Read/write individual Win (identified by "c") and world.winDB.order /
 * world.winDB.active from/to "file" up to the world.delim delimiter. Use "line"
 * and "linemax" as expected by try_fgets().
 *
 * Note that read_winconf_from_file() returns 1 on success and 0 if it detects
 * having found the end of the valid interface configuration file by either
 * hitting a EOF or a newline (so empty newlines at the end of the file are ok).
 *
 * Note that read_order_wins_visible_active() only reads the promised values
 * into pointers for temporary storage. This is due to the order in which window
 * data is initialized: winDB.order and winDB.active should only be set when all
 * windows have been initialized, so cleaning up on error is not confused.
 */
extern uint8_t read_winconf_from_file(char * line, uint32_t linemax,
                                      FILE * file);
extern void write_winconf_of_id_to_file(FILE * file, char c);
extern void read_order_wins_visible_active(char * line, uint32_t linemax,
                                           FILE * file, char ** tmp_order,
                                           char * tmp_active);
extern void write_order_wins_visible_active(FILE * file);

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

/* Toggle display of a window of "id". */
extern void toggle_window(char id);

/* Toggle "window configuration" view for active window. Sets sensible .center
 * values for each configuration view (for winconf_geometry: y=0, x=0; for
 * winconf_keys: x=0 (y is set by draw_winconf_keybindings()); stores default
 * view's .center in .target_center to return to it when toggling back.
 */
extern void toggle_winconfig();

/* Toggle active window's .target_(height/width)_type ("axis" = "y": height;
 * else: width). Don't toggle to .target_width_type of 1 (saving the width as a
 * diff to the .t_screen's width) if window's width is larger than .t_screen's
 * width, for such width is better saved directly with .target_width_type of 0.
 */
extern void toggle_win_size_type(char axis);

/* Grow or shrink active window horizontally ("change" = "*"/"_") or vertically
 * ("change" = "+"/"-") if the new size was at least 1x1, the height at least
 * one cell smaller than .v_screen's vertical hight (to provide space for the
 * title bar) and the width max. (2^16) - 1 cells. If a new window width would
 * surpass that of .t_screen, set active window's .target_width_type to 0.
 */
extern void resize_active_win(char c);

/* Move active window forwards ("dir" == "f") or backwards (any other "dir") in
 * window chain. Wrap around in the window chain if start / end of it is met.
 */
extern void shift_active_win(char dir);

/* Sroll .v_screen one cell to the left if "dir" is "-" and .v_screen_offset is
 * more than 1, or to the right if "dir" is "+" and .v_screen's right edge would
 * not move (further, if suspension of windows has moved it to the left already)
 * leftwards to .t_screen's right edge.
 */
extern void scroll_v_screen(char dir);

/* Cycle active window selection forwards ("dir" == "f") or backwards (any
 * other "dir"). Wrap around in the windows chain if start / end of it is met.
 */
extern void cycle_active_win(char dir);



#endif
