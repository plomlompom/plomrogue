/* windows.c */

#include "windows.h"
#include <stdint.h>    /* for uint8_t, uint16_t, uint32_t */
#include <ncurses.h>   /* for typedefs WINDOW, chtype, wresize(), getmaxx(), */
                       /* getmaxy(), supbad(), delwin(), mvwaddch(),         */
                       /* mvwaddstr(), newpad(), wnoutrefres(), erase(),     */
                       /* werase(), pnoutrefresh(), doupdate()               */
#include <stdlib.h>    /* for malloc(), free() */
#include <string.h>    /* for strlen(), strnlen(), memcpy() */
#include "yx_uint16.h" /* for yx_uint16 coordinates */



/* Fit virtual screen's width to minimum width demanded by current windows'
 * geometries. Returns 0 on success, 1 on (pad memory allocation) error.
 */
static uint8_t refit_pad(struct WinMeta * wmeta);



/* Update geometry (sizes, positions) of window "w" and its successors in the
 * window chain. For the positioning algorithm place_win() is used.
 *
 * update_wins() returns 0 on success, 1 on (pad/window memory alloc.) error.
 */
static uint8_t update_wins(struct WinMeta * wmeta, struct Win * w);
static void place_win(struct WinMeta * wmeta, struct Win * w);



/* Destroy window "w"'s ncurses window (and set w.Frame.curses_win to 0). */
static void destroy_win(struct Win * w);



/* Draw contents of all windows in window chain from window "w" onwards. */
static void draw_wins(struct Win * w);



/* draw_win_borderlines() draws the vertical and horizontal borders of window
 * "w" sans corners into the virtual screen "pad", and draws the top border
 * line as the windows' title bar (highlighted if the window is described
 * active by "active" being set). draw_wins_borderlines().
 *
 * draw_wins_borderlines() calls draw_win_borderlines() recursively on all
 * windows from "w" on. "w_active" is a pointer to the one window that
 * draw_win_borderlines() is supposed to handle as the active window.
 *
 * Finally, draw_wins_bordercorners draws into "pad" the borders of window "w"
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
    uint16_t lastwincol = 0;
    struct Win * w_p = wmeta->chain_start;
    while (w_p != 0)
    {
        if (w_p->start.x + w_p->frame.size.x > lastwincol + 1)
        {
            lastwincol = w_p->start.x + w_p->frame.size.x - 1;
        }
        w_p = w_p->next;
    }

    /* Only resize the pad if the rightmost window column has changed. */
    if (getmaxx(wmeta->padframe.curses_win) != lastwincol)
    {
        return (ERR == wresize(wmeta->padframe.curses_win,
                               getmaxy(wmeta->padframe.curses_win),
                               lastwincol + 2));
    }
    return 0;
}



static uint8_t update_wins(struct WinMeta * wmeta, struct Win * w)
{
    if (0 != w->frame.curses_win)
    {
        destroy_win(w);
    }
    place_win(wmeta, w);
    if (0 != refit_pad(wmeta))
    {
        return 1;
    }
    WINDOW * test = subpad(wmeta->padframe.curses_win,
                           w->frame.size.y, w->frame.size.x,
                           w->start.y, w->start.x);
    if (NULL == test)
    {
        return 1;
    }
    w->frame.curses_win = test;
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
        uint16_t w_prev_maxy = w->prev->start.y
                               + getmaxy(w->prev->frame.curses_win);
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
                w_prev_maxy = w_upup->start.y
                              + getmaxy(w_upup->frame.curses_win);
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



static void destroy_win(struct Win * w)
{
    delwin(w->frame.curses_win);
    w->frame.curses_win = 0;
}



static void draw_wins(struct Win * w)
{
    w->draw(w);
    if (0 != w->next)
    {
        draw_wins(w->next);
    }
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
        }                                     /* +2 is for padding/decoration */
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



extern uint8_t init_win_meta(WINDOW * screen, struct WinMeta * wmeta)
{
    wmeta->screen              = screen;
    wmeta->padframe.size.y     = getmaxy(screen);
    wmeta->padframe.size.x     = getmaxx(screen);
    wmeta->chain_start         = 0;
    wmeta->chain_end           = 0;
    wmeta->pad_offset          = 0;
    WINDOW * test;
    test = newpad(wmeta->padframe.size.y, 1);
    if (NULL == test)
    {
        return 1;
    }
    wmeta->padframe.curses_win = test;
    wmeta->active              = 0;
    return 0;
}



extern struct Win init_win(struct WinMeta * wmeta, char * title,
                           uint16_t height, uint16_t width,
                           void * data, void * func)
{
    struct Win w;
    w.prev             = 0;
    w.next             = 0;
    w.frame.curses_win = 0;
    w.title            = title;
    w.data             = data;
    w.draw             = func;
    if (width > 0)
    {
        w.frame.size.x = width;
    }
    else
    {
        w.frame.size.x = 1;
    }
    if (height > 0 && height <= wmeta->padframe.size.y - 1)
    {
        w.frame.size.y = height;
    }
    else
    {
        w.frame.size.y = wmeta->padframe.size.y - 1;
    }
    return w;
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
    destroy_win(w);

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
        if (0 != update_wins(wmeta, w->next)) /* Positioning of successor    */
        {                                     /* windows may be affected /    */
            return 1;                         /* need correction.             */
        }                                     /* Note that update_wins()      */
        pad_refitted = 1;                     /* already refits the  pad,     */
    }                                         /* voiding later need for that. */
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
        return update_wins(wmeta, wmeta->chain_start); /* Succeeding windows' */
    }                                                  /* positioning may be  */
    return 0;                                          /* affected.           */
}                                                      /* TODO: Why start at  */
                                                       /* chain_start then?   */


extern void cycle_active_win(struct WinMeta * wmeta, char dir)
{
    if (0 != wmeta->active)
    {
        if ('n' == dir)
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



extern void shift_active_win(struct WinMeta * wmeta, char dir)
{
    if (   0 == wmeta->active                     /* No shifting with less    */
        || wmeta->chain_start == wmeta->chain_end /* than two windows visible */
        || (dir != 'f' && dir != 'b'))            /* or wrong direction char. */
    {
        return;
    }
    if ('f' == dir)
    {
        shift_win_forward(wmeta);
    }
    else
    {
        shift_win_backward(wmeta);
    }
    update_wins(wmeta, wmeta->chain_start);
}



extern void draw_all_wins(struct WinMeta * wmeta)
{
    /* Empty everything before filling it a-new. */
    erase();
    wnoutrefresh(wmeta->screen);
    werase(wmeta->padframe.curses_win);
    if (wmeta->chain_start)
    {

        /* Draw windows' contents first, then their borders. */
        draw_wins(wmeta->chain_start);
        draw_wins_borderlines(wmeta->chain_start, wmeta->active,
                              wmeta->padframe.curses_win);
        draw_wins_bordercorners(wmeta->chain_start, wmeta->padframe.curses_win);

        /* Draw virtual screen scroll hints. */
        if (wmeta->pad_offset > 0)
        {
            draw_scroll_hint(&wmeta->padframe,
                             wmeta->pad_offset, wmeta->pad_offset + 1, '<');
        }
        if (wmeta->pad_offset + wmeta->padframe.size.x
            < getmaxx(wmeta->padframe.curses_win) - 1)
        {
            draw_scroll_hint(&wmeta->padframe,
                             wmeta->pad_offset + wmeta->padframe.size.x - 1,
                             getmaxx(wmeta->padframe.curses_win)
                             - (wmeta->pad_offset + wmeta->padframe.size.x),
                             '>');
        }

        /* Write virtual screen segment to be shown on physical screen into */
        /* ncurses screen buffer. */
        pnoutrefresh(wmeta->padframe.curses_win, 0, wmeta->pad_offset, 0, 0,
                     wmeta->padframe.size.y, wmeta->padframe.size.x-1);
    }

    /* Only at the end write accumulated changes to the physical screen. */
    doupdate();
}



extern uint8_t draw_scroll_hint(struct Frame * frame, uint16_t pos,
                                uint32_t dist, char dir)
{
    /* Decide on alignment (vertical/horizontal?), thereby scroll hint text. */
    char * more = "more";
    char * unit_cols = "columns";
    char * unit_rows = "lines";
    uint16_t dsc_space = frame->size.x;
    char * unit = unit_rows;
    if ('<' == dir || '>' == dir)
    {
        dsc_space = frame->size.y;
        unit = unit_cols;
    }
    char * scrolldsc = malloc((4 * sizeof(char)) + strlen(more) + strlen(unit)
                              + 10);                /* 10 = uint32 max strlen */
    if (NULL == scrolldsc)
    {
        return 1;
    }
    sprintf(scrolldsc, " %d %s %s ", dist, more, unit);

    /* Decide on offset of the description text inside the scroll hint line. */
    char offset = 1, q;
    if (dsc_space > strlen(scrolldsc) + 1)
    {
        offset = (dsc_space - strlen(scrolldsc)) / 2;
    }

    /* Draw scroll hint line as dir symbols bracketing description text. */
    chtype symbol;
    for (q = 0; q < dsc_space; q++)
    {
        if (q >= offset && q < strlen(scrolldsc) + offset)
        {
            symbol = scrolldsc[q - offset] | A_REVERSE;
        }
        else
        {
            symbol = dir | A_REVERSE;
        }
        if ('<' == dir || '>' == dir)
        {
            mvwaddch(frame->curses_win, q, pos, symbol);
        }
        else
        {
            mvwaddch(frame->curses_win, pos, q, symbol);
        }
    }

    free(scrolldsc);
    return 0;
}
