/* windows.h
 *
 * A tiled window manager for the terminal.
 *
 * It provides a virtual screen that can be scrolled horizontally and may
 * contain any number of windows that can be appeared, disappeared, resized and
 * (somewhat) moved around. They have borders and a title bar and are positioned
 * (in a bizarre fashion, see below) automatically.
 *
 * Windows can be almost any width (number has to fit into 16 bits); the virtual
 * screen grows with them as needed -- but only horizontally and only up to 2^16
 * cells. Their height is limited by the height of the terminal screen, which
 * must also fit into 2^16 cells.
 *
 * Positioning of windows can only indirectly be influenced: by resizing them,
 * and by shifting their relative position inside the (currently invisible)
 * chain that the window manager treats their plurality as. The first window
 * goes into the upper left corner of the virtual screen. Further visible
 * windows are fitted left-aligned below their (chain-wise) closest predecessor
 * that thrones over enough space to fit them in; failing that, they are placed
 * to the right of the window with the rightmost border.
 *
 * Functions that return uint8_t return these error codes:
 * 0 - success
 * 1 - memory allocation error
 * 2 - would force virtual screen to grow beyond width or height of 2^16 cells
 *
 * TODO: Think up a more intuitive window positioning algorithm or at least make
 * the chain that windows are positioned by visible.
 */

#ifndef WINDOWS_H
#define WINDOWS_H

#include <stdint.h>    /* for uint8_t, uint16_t, uint32_t */
#include <ncurses.h>   /* for the WINDOW and chtype typedefs */
#include "yx_uint16.h" /* for yx_uint16 struct */



/* Individual windows are represented by "Win" structs. They describe frames
 * located inside the virtual screen pad through which "winmaps" are visible,
 * 2-dimensional maps of ncurses chtypes. Win structs are chained into a linked
 * list of all the windows visible on the virtual screen and also contain
 * pointers to what content is to be drawn inside the window, and by use of what
 * method.
 */
struct Win
{
    struct Win * prev;  /* chain pointers; if 0, they mark the start or end  */
    struct Win * next;  /* of the chain; if both are 0, Win is outside chain */
    struct yx_uint16 framesize;   /* window frame size to see winmap through */
    struct yx_uint16 start;       /* upper left corner of window in pad */
    struct yx_uint16 center;      /* window content center to focus window on */
    char * title;                 /* title to be used in window title bar */
    void (* draw) (struct Win *); /* how to draw window content ("data") */
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



/* Initialize empty WinMeta "wmeta" on the terminal "screen". (Note that
 * emptiness is marked by WinMeta.chain_start=0.) Other struct members are also
 * initialized 0, except for the virtual screen (height = that of the terminal
 * screen; width = 1) sized to the size of the terminal screen.
 */
extern uint8_t init_win_meta(WINDOW * screen, struct WinMeta ** wmeta);



/* Initialize a window child of "wmeta" to "title", "height" and "width" and
 * appointing "func"() to interpret and draw the window when it's visible.
 *
 * Pass 0 for "width" to make the window as wide as the terminal screen. Pass
 * negative values for "width" to make the width so many cells smaller than that
 * of the terminal screen. Pass 0 for "height" to give the window the maximum
 * allowed height: one cell smaller than that of the terminal screen. Pass
 * negative values to make the window width so many cells smaller than that of
 * the terminal screen. The maximum allowed height is also applied for positive
 * values that exceed it or negative values that would reduce the window height
 * < 1 cell.
 *
 * Other members of the Win struct are initialized to 0.
 */
extern uint8_t init_win(struct WinMeta * wmeta, struct Win ** w, char * title,
                        int16_t height, int16_t width, void * func);



/* Free allocated memory for an initialized Win / WinMeta structs. */
extern void free_winmeta(struct WinMeta * wmeta);
extern void free_win(struct Win * win);



/* Append/suspend window "w" to/from chain of visible windows below "wmeta".
 * Appended windows will become active. Suspended active windows will move the
 * active window selection to their successor in the window chain or, failing
 * that, their predecessor; if no window remains, none will be active.
 */
extern uint8_t append_win(struct WinMeta * wmeta, struct Win * w);
extern uint8_t suspend_win(struct WinMeta * wmeta, struct Win * w);



/* Apply scrolling offset "new_offset" to virtual screen if it is sane, i.e.
 * it's equal/greater zero and does not push the view (further) beyond the
 * virtual screen's border. If the view is already beyond the virtual screen's
 * border due to it having shrunk after suspension of windows, only allow view
 * movement leftwards.
 */
extern void reset_pad_offset(struct WinMeta * wmeta, uint16_t new_offset);



/* Apply new size "size" to the active window, but only if it provides for at
 * least one cell width/height and is in height at least one cell smaller than
 * the screen's vertical height (to provide space for the title bar). Does
 * nothing if no window is active.
 */
extern uint8_t resize_active_win(struct WinMeta * wmeta, struct yx_uint16 size);



/* Cycle active window selection forwards ("dir" == "f") or backwards (any
 * other "dir"). Wrap around in the windows chain if start / end of it is met.
 * Does nothing if no window is active.
 */
extern void cycle_active_win(struct WinMeta * wmeta, char dir);



/* Move active window forwards ("dir" == "f") or backwards (any other "dir").
 * Wrap around in the window chain if start / end of it is met. Does nothing if
 * no window is active.
 */
extern uint8_t shift_active_win(struct WinMeta * wmeta, char dir);



/* Draw virtual screen including all windows. Also add scroll hints for where
 * the edges of the terminal screen hit non-edges of and inside the virtual
 * screen. Then update the terminal screen.
 */
extern uint8_t draw_all_wins(struct WinMeta * wm);



#endif
