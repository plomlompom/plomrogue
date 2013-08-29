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
 * 1 - memory allocation error (of ncurses' pads/windows, or scroll hint texts)
 * 2 - would force virtual screen to grow beyond width or height of 2^16 cells
 *
 * TODO: Expose less internals to the API.
 *
 * TODO: Think up a more intuitive window positioning algorithm or at least make
 * the chain that windows are positioned by visible.
 */

#ifndef WINDOWS_H
#define WINDOWS_H



#include <stdint.h>    /* for uint8_t, uint16_t, uint32_t */
#include <ncurses.h>   /* for the WINDOW typedef */
#include "yx_uint16.h" /* for yx_uint16 coordinates */



/* Individual windows consist of potential (real only if window is visible
 * inside the virtual screen) ncurses WINDOWs wrapped inside Frame structs (that
 * keep a window's designated size even when it is invisible) wrapped inside
 * metadata-rich Win structs. Win structs are chained into a linked list of all
 * the windows visible on the virtual screen and also contain pointers to what
 * content is to be drawn inside the window, and by use of what method.
 */

struct Frame               /* If Frame is Win's "frame", "size" is the        */
{                          /* designated size of curses_win's ncurses WINDOW. */
    WINDOW * curses_win;   /* If Frame is WinMeta's "padframe", curses_win is */
    struct yx_uint16 size; /* the ncurses pad representing the virtual screen */
};                         /* and "size" desribes the terminal screen size.   */

struct Win
{
    /* members supposed to be used ONLY INTERNALLY */
    struct Win * _prev;  /* chain pointers; if 0, they mark the start or end  */
    struct Win * _next;  /* of the chain; if both are 0, Win is outside chain */
    struct yx_uint16 _start;       /* upper left corner of "frame" WINDOW */
    char * _title;                 /* title to be used in window title bar */
    void (* _draw) (struct Win *); /* how to draw window content ("data") */

    /* members to be available EXTERNALLY */
    struct Frame frame;
    void * data;                   /* window content to be drawn by _draw() */
};



/* The window manager's parent struct WinMeta contains the virtual screen,
 * relates it to the terminal screen and anchors the chain of visible windows.
 */
struct WinMeta
{
    /* members supposed to be used ONLY INTERNALLY */
    WINDOW * _screen;          /* ncurses' pointer to the terminal screen */
    struct Win * _chain_start; /* first Win in chain; its _prev == 0 */
    struct Win * _chain_end;   /* last Win in chain; its _next == 0 */

    /* members to be available EXTERNALLY */
    uint16_t pad_offset;       /* number of cells view is moved to the right */
    struct Frame padframe;     /* virtual screen fitted into terminal screen */
    struct Win * active;       /* Win highlighted/selected for manipulation */
};



/* Initialize empty WinMeta "wmeta" on the terminal "screen". (Note that
 * emptiness is marked by WinMeta.chain_start=0.) Other struct members are also
 * initialized 0, except for the virtual screen (height = that of the terminal
 * screen; width = 1) amd its frame sized to the size of the terminal screen.
 */
extern uint8_t init_win_meta(WINDOW * screen, struct WinMeta * wmeta);



/* Create a window as child of "wmeta" titled "title" of "height" and "width"
 * and appointing "func"() to interpret and draw the content stored at "data"
 * if the window is visible.
 *
 * Pass 0 for "width" to make the window as wide as the terminal screen. Pass
 * negative values for "width" to make the size so many cells smaller than the
 * terminal screen. Pass 0 for "height" to give the window the maximum allowed
 * height: one cell smaller than the terminal screen. Pass negative values to
 * make the window so many smalls smaller than the terminal screen. The maximum
 * allowed height is also applied for positive values that exceed it or negative
 * values that would reduce the window height < 1 cell.
 *
 * Other members of the Win struct are initialized to 0. The window will stay
 * invisible until appended to the chain of visible windows via append_win().
 */
extern struct Win init_win(struct WinMeta * wmeta, char * title,
                           int16_t height, int16_t width,
                           void * data, void * func);



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



/* Draw virtual screen including all windows. Also add scroll hints (see comment
 * on draw_scroll_hint()) for where the edges of the terminal screen hit
 * non-edges of and inside the virtual screen. Then update the terminal screen.
 */
extern uint8_t draw_all_wins(struct WinMeta * wmeta);



/* Draw scroll hint (a line stating that there is more to see on scrolling
 * further into a certain direction) into "frame" at position "pos" (describing
 * a column or a row dependent on "dir" being *either* "<"/">" *or* something
 * else). It will consist of a line of "dir" symbols bracketing a descriptive
 * text stating the number of rows/columns further available beyond the hint.
 */
extern uint8_t draw_scroll_hint(struct Frame * frame, uint16_t pos,
                                uint32_t dist, char dir);



#endif
