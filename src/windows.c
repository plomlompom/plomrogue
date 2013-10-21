/* windows.c */

#include "windows.h"
#include <stdint.h>    /* for uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <ncurses.h>   /* for typedefs WINDOW, chtype, wresize(), getmaxx(), */
                       /* getmaxy(), delwin(), mvwaddch(), mvwaddstr(),      */
                       /* newpad(), wnoutrefres(), erase(), werase(),        */
                       /* pnoutrefresh(), doupdate(), getmaxyx()   */
#include <stdlib.h>    /* for malloc(), free() */
#include <string.h>    /* for strlen(), strnlen(), memcpy() */
#include "yx_uint16.h" /* for struct yx_uint16 */
#include "misc.h"      /* for center_offset() */



/* Fit virtual screen's width to minimum width demanded by current windows'
 * geometries.
 */
static uint8_t refit_pad(struct WinMeta * wmeta);



/* Update geometry (sizes, positions) of window "w" and its successors in the
 * window chain. For the positioning algorithm place_win() is used.
 */
static uint8_t update_wins(struct WinMeta * wmeta, struct Win * w);
static void place_win(struct WinMeta * wmeta, struct Win * w);



/* Draw scroll hint (a line stating that there are "dist" more elements of
 * "unit" further into the direction symbolized by the "dir" char) into virtual
 * screen pad, onto an appropriate edge of "frame": the left or right edge if
 * "dir" is "<" or ">", or the upper or lower edge if it is "^" or "v". "start"
 * should be either the start coordinate of "frame" if it describes a window or
 * .y=, .x=wm->pad_offset if it describes the virtual screen. winscroll_hint()
 * and padscroll_hint() are wrappers to simplify the use of scroll_hint().
 */
static void scroll_hint(struct WinMeta * wm, struct Frame * frame, char dir,
                        uint16_t dist, char * unit, struct yx_uint16 start);
static void winscroll_hint(struct WinMeta * wm, struct Win * w, char dir,
                           uint16_t dist);
static void padscroll_hint(struct WinMeta * wm, char dir, uint16_t dist);



/* Draw contents of all windows in window chain from window "w" onwards. */
static uint8_t draw_wins(struct WinMeta * wm, struct Win * w);



/* draw_win_borderlines() draws the vertical and horizontal borders of window
 * "w" sans corners into the virtual screen "pad", and draws the top border
 * line as the windows' title bar (highlighted if the window is described
 * active by "active" being == 1).
 *
 * draw_wins_borderlines() calls draw_win_borderlines() recursively on all
 * windows from "w" on. "w_active" is a pointer to the one window that
 * draw_win_borderlines() is supposed to handle as the active window.
 *
 * Finally, draw_wins_bordercorners() draws into "pad" the borders of window "w"
 * and all its successors.
 */
static void draw_win_borderlines(struct Win * w, char active, WINDOW * pad);
static void draw_wins_borderlines(struct Win * w, struct Win * w_active,
                                  WINDOW * pad);
static void draw_wins_bordercorners(struct Win * w, WINDOW * pad);



/* Shift active window forwards / backwards in window chain. */
static void shift_win_forward(struct WinMeta * wmeta);
static void shift_win_backward(struct WinMeta * wmeta);



static uint8_t refit_pad(struct WinMeta * wmeta)
{
    /* Determine rightmost window column. */
    uint32_t lastwincol = 0;
    struct Win * w_p = wmeta->chain_start;
    while (w_p != 0)
    {
        if ((uint32_t) w_p->start.x + (uint32_t) w_p->frame.size.x
            > lastwincol + 1)
        {
            lastwincol = (uint32_t) w_p->start.x
                         + (uint32_t) w_p->frame.size.x - 1;
        }
        w_p = w_p->next;
    }

    /* Only resize the pad if the rightmost window column has changed. */
    if (getmaxx(wmeta->padframe.curses_win) + 1 != lastwincol)
    {
        if (lastwincol + 2 > UINT16_MAX)
        {
            return 2;
        }
        return (ERR == wresize(wmeta->padframe.curses_win,
                               getmaxy(wmeta->padframe.curses_win),
                               lastwincol + 2));
    }
    return 0;
}



static uint8_t update_wins(struct WinMeta * wmeta, struct Win * w)
{
    place_win(wmeta, w);
    uint8_t test_refit = refit_pad(wmeta);
    if (0 != test_refit)
    {
        return test_refit;
    }
    if (0 != w->next)
    {
        return update_wins(wmeta, w->next);
    }
    return 0;
}



static void place_win(struct WinMeta * wmeta, struct Win * w)
{
    /* First window goes into the upper-left corner. */
    w->start.x = 0;
    w->start.y = 1;                             /* Leave space for title bar. */
    if (0 != w->prev)
    {

        /* Non-first window fallbacks to: fit rightwards of rightmost border. */
        struct Win * w_top = w->prev;
        while (w_top->start.y != 1)
        {
            w_top = w_top->prev;
        }
        w->start.x = w_top->start.x + w_top->frame.size.x + 1;

        /* Fit window below its predecessor if that one directly thrones over
         * empty space wide and high enough.
         */
        uint16_t w_prev_maxy = w->prev->start.y + w->prev->frame.size.y;
        if (   w->frame.size.x <= w->prev->frame.size.x
            && w->frame.size.y <  wmeta->padframe.size.y - w_prev_maxy)
        {
            w->start.x = w->prev->start.x;
            w->start.y = w_prev_maxy + 1;
        }

        /* Failing that, try to open a new sub column below the nearest
         * predecessor window that thrones over enough empty space.
         */
        else
        {
            struct Win * w_up = w->prev;
            struct Win * w_upup = w_up;
            uint16_t widthdiff;
            while (w_up != w_top)
            {
                w_upup = w_up->prev;
                while (1)
                {
                    if (w_up->start.y != w_upup->start.y)
                    {
                        break;
                    }
                    w_upup = w_upup->prev;
                }
                w_prev_maxy = w_upup->start.y + w_upup->frame.size.y;
                widthdiff = (w_upup->start.x + w_upup->frame.size.x)
                            - (w_up->start.x + w_up->frame.size.x);
                if (   w->frame.size.y < wmeta->padframe.size.y - w_prev_maxy
                    && w->frame.size.x < widthdiff)
                {
                    w->start.x = w_up->start.x + w_up->frame.size.x + 1 ;
                    w->start.y = w_prev_maxy + 1;
                    break;
                }
                w_up = w_upup;
            }
        }
    }
}



static void scroll_hint(struct WinMeta * wm, struct Frame * frame, char dir,
                        uint16_t dist, char * unit, struct yx_uint16 start)
{
    /* Decide on alignment (vertical/horizontal?), thereby hint text space. */
    char * more = "more";
    uint16_t dsc_space = frame->size.x;
    if ('<' == dir || '>' == dir)
    {
        dsc_space = frame->size.y;
    }                                  /* vv-- 10 = max strlen for uint16_t */
    char scrolldsc[1 + strlen(more) + 1 + 10 + 1 + strlen(unit) + 1 + 1];
    sprintf(scrolldsc, " %d %s %s ", dist, more, unit);

    /* Decide on offset of the description text inside the scroll hint line. */
    uint16_t dsc_offset = 1;
    if (dsc_space > strlen(scrolldsc) + 1)
    {
        dsc_offset = (dsc_space - strlen(scrolldsc)) / 2;
    }

    /* Draw scroll hint line as dir symbols bracketing description text. */
    uint16_t draw_offset = 0;
    if      ('>' == dir)
    {
        draw_offset = frame->size.x - 1;
    }
    else if ('v' == dir)
    {
        draw_offset = frame->size.y - 1;
    }
    uint16_t q = 0;
    for (; q < dsc_space; q++)
    {
        chtype symbol = dir | A_REVERSE;
        if (q >= dsc_offset && q < strlen(scrolldsc) + dsc_offset)
        {
            symbol = scrolldsc[q - dsc_offset] | A_REVERSE;
        }
        if ('<' == dir || '>' == dir)
        {
            mvwaddch(wm->padframe.curses_win,
                     start.y + q, start.x + draw_offset, symbol);
        }
        else
        {
            mvwaddch(wm->padframe.curses_win,
                     start.y + draw_offset, start.x + q, symbol);
        }
    }
}


static void padscroll_hint(struct WinMeta * wm, char dir, uint16_t dist)
{
    struct yx_uint16 start;
    start.y = 0;
    start.x = wm->pad_offset;
    scroll_hint(wm, &wm->padframe, dir, dist, "columns", start);
}



static void winscroll_hint(struct WinMeta * wm, struct Win * w, char dir,
                           uint16_t dist)
{
    char * unit = "lines";
    if ('<' == dir || '>' == dir)
    {
        unit = "columns";
    }
    struct yx_uint16 start = w->start;
    scroll_hint(wm, &w->frame, dir, dist, unit, start);
}



static uint8_t draw_wins(struct WinMeta * wm, struct Win * w)
{
    if (ERR == wresize(w->frame.curses_win, 1, 1))
    {
        return 1;
    }
    w->draw(w);
    uint16_t y, x, size_y, size_x;
    getmaxyx(w->frame.curses_win, size_y, size_x);
    uint16_t offset_y = center_offset(w->center.y, size_y, w->frame.size.y);
    uint16_t offset_x = center_offset(w->center.x, size_x, w->frame.size.x);
    for (y = offset_y; y < w->frame.size.y + offset_y && y < size_y; y++)
    {
        for (x = offset_x; x < w->frame.size.x + offset_x && x < size_x; x++)
        {
            chtype ch = mvwinch(w->frame.curses_win, y, x);
            mvwaddch(wm->padframe.curses_win, w->start.y + (y - offset_y),
                                              w->start.x + (x - offset_x), ch);
        }
    }
    if (offset_y > 0)
    {
        winscroll_hint(wm, w, '^', offset_y + 1);
    }
    if (size_y > offset_y + w->frame.size.y)
    {
        winscroll_hint(wm, w, 'v', size_y - ((offset_y + w->frame.size.y) - 1));
    }
    if (offset_x > 0)
    {
        winscroll_hint(wm, w, '<', offset_x + 1);
    }
    if (size_x > offset_x + w->frame.size.x)
    {
        winscroll_hint(wm, w, '>', size_x - ((offset_x + w->frame.size.x) - 1));
    }
    if (0 != w->next)
    {
        return draw_wins(wm, w->next);
    }
    return 0;
}



static void draw_win_borderlines(struct Win * w, char active, WINDOW * pad)
{
    /* Draw vertical and horizontal border lines. */
    uint16_t y, x;
    for (y = w->start.y; y <= w->start.y + w->frame.size.y; y++)
    {
        mvwaddch(pad, y, w->start.x - 1,               '|');
        mvwaddch(pad, y, w->start.x + w->frame.size.x, '|');
    }
    for (x = w->start.x; x <= w->start.x + w->frame.size.x; x++)
    {
        mvwaddch(pad, w->start.y - 1,               x, '-');
        mvwaddch(pad, w->start.y + w->frame.size.y, x, '-');
    }

    /* Draw as much as possible of the title into center of top border line. */
    char min_title_length_visible = 3;  /* min. 1 char + 2 padding/decoration */
    if (w->frame.size.x >= min_title_length_visible)
    {
        uint16_t title_offset = 0;
        if (w->frame.size.x > strlen(w->title) + 2)
        {
            title_offset = (w->frame.size.x - (strlen(w->title) + 2)) / 2;
        }                                    /* +2 is for padding/decoration */
        uint16_t length_visible = strnlen(w->title, w->frame.size.x - 2);
        char title[length_visible + 3];
        char decoration = ' ';
        if (1 == active)
        {
            decoration = '$';
        }
        memcpy(title + 1, w->title, length_visible);
        title[0] = title[length_visible + 1] = decoration;
        title[length_visible + 2] = '\0';
        mvwaddstr(pad, w->start.y - 1, w->start.x + title_offset, title);
    }
}



static void draw_wins_borderlines(struct Win * w, struct Win * w_active,
                                  WINDOW * pad)
{
    char active = 0;
    if (w == w_active)
    {
        active = 1;
    }
    draw_win_borderlines(w, active, pad);
    if (0 != w->next)
    {
        draw_wins_borderlines(w->next, w_active, pad);
    }
}



static void draw_wins_bordercorners(struct Win * w, WINDOW * pad)
{
    mvwaddch(pad, w->start.y - 1, w->start.x - 1, '+');
    mvwaddch(pad, w->start.y - 1, w->start.x + w->frame.size.x, '+');
    mvwaddch(pad, w->start.y + w->frame.size.y, w->start.x - 1, '+');
    mvwaddch(pad,
             w->start.y + w->frame.size.y, w->start.x + w->frame.size.x, '+');
    if (0 != w->next)
    {
        draw_wins_bordercorners(w->next, pad);
    }
}



static void shift_win_forward(struct WinMeta * wmeta)
{
    if (wmeta->active == wmeta->chain_end)
    {
        wmeta->chain_end = wmeta->active->prev;
        wmeta->chain_end->next = 0;
        wmeta->active->next = wmeta->chain_start;
        wmeta->active->next->prev = wmeta->active;
        wmeta->chain_start = wmeta->active;
        wmeta->chain_start->prev = 0;
    }
    else
    {
        struct Win * old_prev = wmeta->active->prev;
        struct Win * old_next = wmeta->active->next;
        if (wmeta->chain_end == wmeta->active->next)
        {
            wmeta->chain_end = wmeta->active;
            wmeta->active->next = 0;
        }
        else
        {
            wmeta->active->next = old_next->next;
            wmeta->active->next->prev = wmeta->active;
        }
        if (wmeta->chain_start == wmeta->active)
        {
            wmeta->chain_start = old_next;
        }
        else
        {
            old_prev->next = old_next;
        }
        old_next->prev = old_prev;
        old_next->next = wmeta->active;
        wmeta->active->prev = old_next;
    }
}



static void shift_win_backward(struct WinMeta * wmeta)
{
    if (wmeta->active == wmeta->chain_start)
    {
        wmeta->chain_start = wmeta->active->next;
        wmeta->chain_start->prev = 0;
        wmeta->active->prev = wmeta->chain_end;
        wmeta->active->prev->next = wmeta->active;
        wmeta->chain_end = wmeta->active;
        wmeta->chain_end->next = 0;
    }
    else
    {
        struct Win * old_prev = wmeta->active->prev;
        struct Win * old_next = wmeta->active->next;
        if (wmeta->chain_start == wmeta->active->prev)
        {
            wmeta->chain_start = wmeta->active;
            wmeta->active->prev = 0;
        }
        else
        {
            wmeta->active->prev = old_prev->prev;
            wmeta->active->prev->next = wmeta->active;
        }
        if (wmeta->chain_end == wmeta->active)
        {
            wmeta->chain_end = old_prev;
        }
        else
        {
            old_next->prev = old_prev;
        }
        old_prev->next = old_next;
        old_prev->prev = wmeta->active;
        wmeta->active->next = old_prev;
    }
}



extern uint8_t init_win_meta(WINDOW * screen, struct WinMeta ** wmp)
{
    struct WinMeta * wmeta = malloc(sizeof(struct WinMeta));
    wmeta->screen              = screen;
    uint32_t maxy_test         = getmaxy(screen);
    uint32_t maxx_test         = getmaxx(screen);
    if (maxy_test > UINT16_MAX || maxx_test > UINT16_MAX)
    {
        return 2;
    }
    wmeta->padframe.size.y     = maxy_test;
    wmeta->padframe.size.x     = maxx_test;
    wmeta->chain_start         = 0;
    wmeta->chain_end           = 0;
    wmeta->pad_offset          = 0;
    WINDOW * pad_test          = newpad(wmeta->padframe.size.y, 1);
    if (NULL == pad_test)
    {
        return 1;
    }
    wmeta->padframe.curses_win = pad_test;
    wmeta->active              = 0;
    *wmp = wmeta;
    return 0;
}



extern uint8_t init_win(struct WinMeta * wmeta, struct Win ** wp, char * title,
                        int16_t height, int16_t width,
                        void * data, void * func)
{
    struct Win * w = malloc(sizeof(struct Win));
    if (NULL == w)
    {
        return 1;
    }
    w->prev             = 0;
    w->next             = 0;
    w->frame.curses_win = newpad(1, 1);
    if (NULL == w->frame.curses_win)
    {
        return 1;
    }
    w->title            = malloc(strlen(title) + 1);
    if (NULL == w->title)
    {
        return 1;
    }
    sprintf(w->title, "%s", title);
    w->data             = data;
    w->draw             = func;
    w->center.y         = 0;
    w->center.x         = 0;
    if      (0 < width)
    {
        w->frame.size.x = width;
    }
    else if (0 >= width)
    {
        w->frame.size.x = wmeta->padframe.size.x + width;
    }
    if      (0 < height && height <= wmeta->padframe.size.y - 1)
    {
        w->frame.size.y = height;
    }
    else if (0 >= height && wmeta->padframe.size.y + (height - 1) > 0)
    {
        w->frame.size.y = wmeta->padframe.size.y + (height - 1);
    }
    *wp = w;
    return 0;
}



extern void free_winmeta(struct WinMeta * wmeta)
{
    delwin(wmeta->padframe.curses_win);
    free(wmeta);
}



extern void free_win(struct Win * win)
{
    delwin(win->frame.curses_win);
    free(win->title);
    free(win);
}



extern uint8_t append_win(struct WinMeta * wmeta, struct Win * w)
{
    if (0 != wmeta->chain_start)
    {
        w->prev = wmeta->chain_end;
        wmeta->chain_end->next = w;
    }
    else
    {
        wmeta->active = w;
        wmeta->chain_start = w;
    }
    wmeta->chain_end = w;
    return update_wins(wmeta, w);
}



extern uint8_t suspend_win(struct WinMeta * wmeta, struct Win * w)
{
    if (wmeta->chain_start != w)
    {
        w->prev->next = w->next;
    }
    else
    {
        wmeta->chain_start = w->next;
    }
    char pad_refitted = 0;
    if (wmeta->chain_end != w)
    {
        w->next->prev = w->prev;
        if (wmeta->active == w)
        {
            wmeta->active = w->next;
        }
        uint8_t test = update_wins(wmeta, w->next); /* Positioning of         */
        if (0 != test)                              /* successor windows may  */
        {                                           /* be affected / need     */
            return test;                            /* correction. Note that  */
        }                                           /* update_wins() already  */
        pad_refitted = 1;                           /* refits the pad, voiding*/
    }                                               /* later need for that.   */
    else
    {
        wmeta->chain_end = w->prev;
        if (wmeta->active == w)
        {
            wmeta->active = w->prev;
        }
    }

    w->prev = 0;
    w->next = 0;

    if (0 == pad_refitted)
    {
        return refit_pad(wmeta);
    }
    return 0;
}



extern void reset_pad_offset(struct WinMeta * wmeta, uint16_t new_offset)
{
    if (new_offset >= 0
        && (new_offset < wmeta->pad_offset
            || new_offset + wmeta->padframe.size.x
               < getmaxx(wmeta->padframe.curses_win)))
    {
        wmeta->pad_offset = new_offset;
    }
}



extern uint8_t resize_active_win(struct WinMeta * wmeta, struct yx_uint16 size)
{
    if (0 != wmeta->active
        && size.x > 0 && size.y > 0
        && size.y < wmeta->padframe.size.y)
    {
        wmeta->active->frame.size = size;
        return update_wins(wmeta, wmeta->active); /* Positioning of following */
    }                                             /* windows may be affected. */
    return 0;
}



extern void cycle_active_win(struct WinMeta * wmeta, char dir)
{
    if (0 != wmeta->active)
    {
        if ('f' == dir)
        {
            if (wmeta->active->next != 0)
            {
                wmeta->active = wmeta->active->next;
            }
            else
            {
                wmeta->active = wmeta->chain_start;
            }
        }
        else
        {
            if (wmeta->active->prev != 0)
            {
                wmeta->active = wmeta->active->prev;
            }
            else
            {
                wmeta->active = wmeta->chain_end;
            }
        }
    }
}



extern uint8_t shift_active_win(struct WinMeta * wmeta, char dir)
{
    if (   0 == wmeta->active                        /* No shifting with < 2  */
        || wmeta->chain_start == wmeta->chain_end)   /* windows visible.      */
    {
        return 0;
    }
    if ('f' == dir)
    {
        shift_win_forward(wmeta);
    }
    else
    {
        shift_win_backward(wmeta);
    }
    return update_wins(wmeta, wmeta->chain_start);
}



extern uint8_t draw_all_wins(struct WinMeta * wm)
{
    /* Empty everything before filling it a-new. */
    erase();
    wnoutrefresh(wm->screen);
    werase(wm->padframe.curses_win);
    if (wm->chain_start)
    {

        /* Draw windows' borders first, then windows. */
        draw_wins_borderlines(wm->chain_start, wm->active,
                              wm->padframe.curses_win);
        draw_wins_bordercorners(wm->chain_start, wm->padframe.curses_win);

        if (1 == draw_wins(wm, wm->chain_start))
        {
            return 1;
        }

        /* Draw virtual screen scroll hints. */
        if (wm->pad_offset > 0)
        {
            padscroll_hint(wm, '<', wm->pad_offset + 1);
        }
        uint16_t size_x = getmaxx(wm->padframe.curses_win);
        uint16_t right_edge = wm->pad_offset + wm->padframe.size.x;
        if (right_edge < size_x - 1)
        {
            padscroll_hint(wm, '>', size_x - right_edge);
        }

        /* Write pad segment to be shown on physical screen to screen buffer. */
        pnoutrefresh(wm->padframe.curses_win, 0, wm->pad_offset, 0, 0,
                     wm->padframe.size.y, wm->padframe.size.x - 1);
    }

    /* Only at the end write accumulated changes to the physical screen. */
    doupdate();
    return 0;
}
