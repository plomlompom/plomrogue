/* windows.h
 *
 * A tiled window manager for the terminal.
 *
 * It provides a virtual screen that can be scrolled horizontally and may carry
 * any number of windows to be appeared, disappeared, resized and moved around.
 * They have borders and a title bar and are positioned automtaically.
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
 * thrones over enough space to to contain them and is open to the right. If
 * that fails, they're fitted up-right to the window with the rightmost border.
 *
 * TODO: Think up a more intuitive window positioning algorithm.
 */

#ifndef WINDOWS_H
#define WINDOWS_H

#include <stdint.h>    /* for uint8_t, uint16_t, uint32_t */
#include <ncurses.h>   /* for the WINDOW and chtype typedefs */
#include "yx_uint16.h" /* for yx_uint16 struct */



/* "Win" structs describe windows as frames located inside the virtual screen
 * pad through which "winmaps" are visible, 2-dimensional maps of ncurses
 * chtypes. If a winmap is bigger than its frame, scrolling hints will appear at
 * the proper edges. Win structs are chained into a linked list of all the
 * windows visible on the virtual screen and also contain pointers to what
 * content is to be drawn inside the window, and by use of what method.
 */
struct Win
{
    struct Win * prev;  /* chain pointers; if 0, they mark the start or end  */
    struct Win * next;  /* of the chain; if both are 0, Win is outside chain */
    struct yx_uint16 framesize;   /* window frame size to see winmap through */
    struct yx_uint16 start;       /* upper left corner of window in pad */
    struct yx_uint16 center;      /* winmap cell to center frame on if smaller*/
    char * title;                 /* title to be used in window title bar */
    void (* draw) (struct Win *); /* function that draws/updates the  winmap */
    chtype * winmap;              /* sequence of cells, sorted into lines ... */
    struct yx_uint16 winmapsize;  /* ... with these geometry infos  */
};

/* The window manager's parent struct WinMeta contains the virtual screen,
 * relates it to the terminal screen and anchors the chain of visible windows.
 */
struct WinMeta
{
    WINDOW * screen;          /* ncurses' pointer to the terminal screen */
    WINDOW * pad;             /* ncurses pad of virtual screen */
    uint16_t pad_offset;      /* number of cells view is moved to the right */
    struct yx_uint16 padsize; /* virtual screen size */
    struct Win * chain_start; /* first Win in chain; its _prev == 0 */
    struct Win * chain_end;   /* last Win in chain; its _next == 0 */
    struct Win * active;      /* Win highlighted/selected for manipulation */
};



/* Initialize empty "wmeta" on terminal "screen". All struct members are
 * initialized to 0, except for the newly created virtual screen "pad" and its
 * size (height: that of the terminal screen; width: 1).
 */
extern void init_win_meta(WINDOW * screen);

/* Initialize a window child of "wmeta" to "title", "height" and "width" and
 * appoint "func"() as its .draw to draw the winmap when it's visible. Other
 * members of the Win struct pointed to by "wp" are initialized to 0.
 *
 * Pass 0 for "width" to make the window as wide as the terminal screen. Pass 0
 * for "height" for the maximum allowed height: one cell smaller than that of
 * the terminal screen. Pass negative values for either of them to make the
 * window width/height so many cells smaller than what 0 would set. The maximum
 * allowed height is also applied for positive height values that exceed it or
 * negative values that would reduce the window height to less than 1 cell.
 *
 * Other members of the Win struct are initialized to 0.
 */
extern void init_win(struct Win ** wp, char * title, int16_t height,
                     int16_t width, void * func);

/* Free allocated memory for an initialized Win / WinMeta structs. */
extern void free_winmeta();
extern void free_win(struct Win * win);

/* Append/suspend window "w" to/from chain of visible windows below "wmeta".
 * Appended windows will become active. Suspended active windows will move the
 * active window selection to their successor in the window chain or, failing
 * that, their predecessor, or, failing that, to 0 (no window being active).
 */
extern void append_win(struct Win * w);
extern void suspend_win(struct Win * w);

/* Apply scrolling offset "new_offset" to virtual screen if it is equal/greater
 * 0 and does not push the view (further) beyond the virtual screen's border. If
 * the view is already beyond the virtual screen's border due to it having
 * shrunk after suspension of windows, only allow screen scrolling leftwards.
 */
extern void reset_pad_offset(uint16_t new_offset);

/* Apply "size" to the active window if it provides a minimum size of 1x1 cells
 * and is in height at least one cell smaller than the screen's vertical height
 * (to provide space for the title bar). Does nothing if no window is active.
 */
extern void resize_active_win(struct yx_uint16 size);

/* Cycle active window selection forwards ("dir" == "f") or backwards (any
 * other "dir"). Wrap around in the windows chain if start / end of it is met.
 * Does nothing if no window is active.
 */
extern void cycle_active_win(char dir);

/* Move active window forwards ("dir" == "f") or backwards (any other "dir") in
 * the window chain. Wrap around in the window chain if start / end of it is
 * met. Does nothing if no window is active.
 */
extern void shift_active_win(char dir);

/* Draw virtual screen including all windows. Also add scroll hints for where
 * the edges of the terminal screen hit non-edges of and inside the virtual
 * screen. Then update the terminal screen.
 */
extern void draw_all_wins();



#endif
