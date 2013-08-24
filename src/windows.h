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
 * screen grows with them as needed -- but only horizontally. Their height is
 * limited by the height of the terminal screen.
 *
 * Positioning of windows can only indirectly be influenced: by resizing them,
 * and by shifting their relative position inside the (currently invisible)
 * chain that the window manager treats their plurality as. The first window
 * goes into the upper left corner of the virtual screen. Further visible
 * windows are fitted left-aligned below their (chain-wise) closest predecessor
 * that thrones over enough space to fit them in; failing that, they are placed
 * to the right of the window with the rightmost border.
 *
 * TODO: Think up a more intuitive window positioning algorithm or at least make
 * the chain that windows are positioned by visible.
 *
 * TODO: Ensure there are only windows as many / as big as fit into the maximum
 * size of the virtual screen.
 */

#ifndef WINDOWS_H
#define WINDOWS_H



#include <stdint.h>    /* for uint16_t, uint32_t */
#include <ncurses.h>   /* for the WINDOW typedef */
#include "yx_uint16.h" /* for yx_uint16 coordinates */


/* Individual windows consist of potential (real if window is visible) ncurses
 * WINDOWs wrapped inside Frame structs (that keep a window's designated size
 * even when it is invisible) wrapped inside metadata-rich Win structs. Win
 * structs are chained into a linked list of all the windows visible on the
 * virtual screen and also contain pointers to what content is to be drawn
 * inside the window, and by use of what method.
 */

struct Frame
{
    WINDOW * curses_win;
    struct yx_uint16 size;  /* the size curses_win fits into; for the virtual */
};                          /* screen padframe: the terminal screen size      */

struct Win
{
    struct Win * prev;            /* prev=next=0 if Win is outside the chain */
    struct Win * next;
    struct yx_uint16 start;       /* upper left corner (of WINDOW or border?) */
    struct Frame frame;
    char * title;                 /* title to be shown on window border top */
    void (* draw) (struct Win *); /* function to draw window content ("data") */
    void * data;                  /* content to be drawn; draw() knows how */
};



/* The window manager's parent struct WinMeta provides the virtual screen and a
 * representation of the terminal screen. It also anchors start and end of the
 * windows chain.
 */
struct WinMeta
{
    WINDOW * screen;          /* terminal screen */
    uint16_t pad_offset;      /* number of cells view is moved to the right */
    struct Frame padframe;    /* virtual screen fitted into terminal screen */
    struct Win * chain_start; /* first Win, whose .prev shall point to 0 */
    struct Win * chain_end;   /* last Win, whose .next shall point to 0 */
    struct Win * active;      /* window highlighted/selected for manipulation */
};



/* Initialize empty WinMeta "wmeta" on the terminal "screen". Note that
 * emptiness is marked by WinMeta.chain_start=0. Other struct values are also
 * initialized 0, except for the virtual screen (terminal screen height, width =
 * 1) and its terminal-sized frame.
 *
 * Returns 0 on success, 1 on (ncurses newpad() memory allocation) error.
 */
extern uint8_t init_win_meta(WINDOW * screen, struct WinMeta * wmeta);



/* Create a window below inside "wmeta" titled "title" of "height" and "width"
 * and appointing "func"() to interpret and draw the content stored at "data"
 * if the window is visible.
 *
 * A value for "width" <1 will trigger a fallback to width=1. A "height" <1 or
 * larger than the maximum window height possible within the virtual screen will
 * trigger a fallback to the maximum height possible (i.e. pass a "height" of 0
 * to initialize the window to its largest possible height).
 *
 * Other values of the Win struct will be initialized to 0. The window will stay
 * invisible until appended to the chain of visible windows via append_win().
 */
extern struct Win init_win(struct WinMeta * wmeta, char * title,
                           uint16_t height, uint16_t widtht,
                           void * data, void * func);



/* Append/suspend window "w" to/from chain of visible windows in "wmeta".
 * Appended windows will become active. Suspended active windows will move the
 * active window selection to their successor in the window chain or, failing
 * that, their predecessor; if no window remains, none will be active.
 *
 * Return 0 on success, and 1 on (ncurses window/pad memory allocation) error.
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
 * the screen's vertical height (to provide space for the title bar).
 *
 * Returns 0 on success, 1 on (ncurses window/pad memory allocation) error.
 */
extern uint8_t resize_active_win(struct WinMeta * wmeta, struct yx_uint16 size);



/* Cycle active window selection forwards (set dir="n") or backwards. Wrap
 * around in the windows chain if start / end of it is met.
 */
extern void cycle_active_win(struct WinMeta * wmeta, char dir);



/* Move active window forwards (set dir="f") or backwards (set dir="b"). Wrap
 * around in the window chain if start / end of it is met.
 *
 * Returns 0 on success, 1 on (ncurses window/pad memory allocation) error.
 */
extern uint8_t shift_active_win(struct WinMeta * wmeta, char dir);



/* Draw virtual screen including all windows. Also add scroll hints (see comment
 * on draw_scroll_hint()) for where the edges of the terminal screen hit
 * non-edges of and inside the virtual screen. Then update the terminal screen.
 *
 * Returns 0 on success, 1 on error (of scroll hint text memory allocation).
 */
extern uint8_t draw_all_wins(struct WinMeta * wmeta);



/* Draw scroll hint (a line stating that there is more to see on scrolling
 * further into a certain direction) into "frame" at position "pos" (describing
 * a column or a row dependent on "dir" being *either* "<"/">" *or* something
 * else). It will consist of a line of "dir" symbols bracketing a descriptive
 * text stating the number of rows/columns further available beyond the hint.
 *
 * Return 0 on success, and 1 on error (of scroll hint text memory allocation).
 */
extern uint8_t draw_scroll_hint(struct Frame * frame, uint16_t pos,
                                uint32_t dist, char dir);



#endif
