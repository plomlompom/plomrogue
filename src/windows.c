/* windows.c */

#include "windows.h"
#include <stdint.h>    /* for uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <ncurses.h>   /* for typedefs WINDOW, chtype, wresize(), getmaxx(), */
                       /* getmaxy(), supbad(), delwin(), mvwaddch(),         */
                       /* mvwaddstr(), newpad(), wnoutrefres(), erase(),     */
                       /* werase(), pnoutrefresh(), doupdate()               */
#include <stdlib.h>    /* for malloc(), free() */
#include <string.h>    /* for strlen(), strnlen(), memcpy() */
#include "yx_uint16.h" /* for yx_uint16 coordinates */



/* Fit virtual screen's width to minimum width demanded by current windows'
 * geometries.
 */
static uint8_t refit_pad(struct WinMeta * wmeta);



/* Update geometry (sizes, positions) of window "w" and its successors in the
 * window chain. For the positioning algorithm place_win() is used.
 */
static uint8_t update_wins(struct WinMeta * wmeta, struct Win * w);
static void place_win(struct WinMeta * wmeta, struct Win * w);



/* Destroy window "w"'s ncurses WINDOW (and set w.Frame.curses_win to 0). */
static void destroy_win(struct Win * w);



/* Draw contents of all windows in window chain from window "w" onwards. */
static void draw_wins(struct Win * w);



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
    struct Win * w_p = wmeta->_chain_start;
    while (w_p != 0)
    {
        if ((uint32_t) w_p->_start.x + (uint32_t) w_p->frame.size.x
            > lastwincol + 1)
        {
            lastwincol = (uint32_t) w_p->_start.x
                         + (uint32_t) w_p->frame.size.x - 1;
        }
        w_p = w_p->_next;
    }

    /* Only resize the pad if the rightmost window column has changed. */
    if (getmaxx(wmeta->padframe.curses_win) != lastwincol)
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
    if (0 != w->frame.curses_win)
    {
        destroy_win(w);
    }
    place_win(wmeta, w);
    uint8_t test_refit = refit_pad(wmeta);
    if (0 != test_refit)
    {
        return test_refit;
    }
    WINDOW * subpad_test = subpad(wmeta->padframe.curses_win,
                                  w->frame.size.y, w->frame.size.x,
                                  w->_start.y, w->_start.x);
    if (NULL == subpad_test)
    {
        return 1;
    }
    w->frame.curses_win = subpad_test;
    if (0 != w->_next)
    {
        return update_wins(wmeta, w->_next);
    }
    return 0;
}



static void place_win(struct WinMeta * wmeta, struct Win * w)
{
    /* First window goes into the upper-left corner. */
    w->_start.x = 0;
    w->_start.y = 1;                            /* Leave space for title bar. */
    if (0 != w->_prev)
    {

        /* Non-first window fallbacks to: fit rightwards of rightmost border. */
        struct Win * w_top = w->_prev;
        while (w_top->_start.y != 1)
        {
            w_top = w_top->_prev;
        }
        w->_start.x = w_top->_start.x + w_top->frame.size.x + 1;

        /* Fit window below its predecessor if that one directly thrones over
         * empty space wide and high enough.
         */
        uint16_t w_prev_maxy = w->_prev->_start.y
                               + getmaxy(w->_prev->frame.curses_win);
        if (   w->frame.size.x <= w->_prev->frame.size.x
            && w->frame.size.y <  wmeta->padframe.size.y - w_prev_maxy)
        {
            w->_start.x = w->_prev->_start.x;
            w->_start.y = w_prev_maxy + 1;
        }

        /* Failing that, try to open a new sub column below the nearest
         * predecessor window that thrones over enough empty space.
         */
        else
        {
            struct Win * w_up = w->_prev;
            struct Win * w_upup = w_up;
            uint16_t widthdiff;
            while (w_up != w_top)
            {
                w_upup = w_up->_prev;
                while (1)
                {
                    if (w_up->_start.y != w_upup->_start.y)
                    {
                        break;
                    }
                    w_upup = w_upup->_prev;
                }
                w_prev_maxy = w_upup->_start.y
                              + getmaxy(w_upup->frame.curses_win);
                widthdiff = (w_upup->_start.x + w_upup->frame.size.x)
                            - (w_up->_start.x + w_up->frame.size.x);
                if (   w->frame.size.y < wmeta->padframe.size.y - w_prev_maxy
                    && w->frame.size.x < widthdiff)
                {
                    w->_start.x = w_up->_start.x + w_up->frame.size.x + 1 ;
                    w->_start.y = w_prev_maxy + 1;
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
    w->_draw(w);
    if (0 != w->_next)
    {
        draw_wins(w->_next);
    }
}



static void draw_win_borderlines(struct Win * w, char active, WINDOW * pad)
{
    /* Draw vertical and horizontal border lines. */
    uint16_t y, x;
    for (y = w->_start.y; y <= w->_start.y + w->frame.size.y; y++)
    {
        mvwaddch(pad, y, w->_start.x - 1,               '|');
        mvwaddch(pad, y, w->_start.x + w->frame.size.x, '|');
    }
    for (x = w->_start.x; x <= w->_start.x + w->frame.size.x; x++)
    {
        mvwaddch(pad, w->_start.y - 1,               x, '-');
        mvwaddch(pad, w->_start.y + w->frame.size.y, x, '-');
    }

    /* Draw as much as possible of the title into center of top border line. */
    char min_title_length_visible = 3;  /* min. 1 char + 2 padding/decoration */
    if (w->frame.size.x >= min_title_length_visible)
    {
        uint16_t title_offset = 0;
        if (w->frame.size.x > strlen(w->_title) + 2)
        {
            title_offset = (w->frame.size.x - (strlen(w->_title) + 2)) / 2;
        }                                    /* +2 is for padding/decoration */
        uint16_t length_visible = strnlen(w->_title, w->frame.size.x - 2);
        char title[length_visible + 3];
        char decoration = ' ';
        if (1 == active)
        {
            decoration = '$';
        }
        memcpy(title + 1, w->_title, length_visible);
        title[0] = title[length_visible + 1] = decoration;
        title[length_visible + 2] = '\0';
        mvwaddstr(pad, w->_start.y - 1, w->_start.x + title_offset, title);
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
    if (0 != w->_next)
    {
        draw_wins_borderlines(w->_next, w_active, pad);
    }
}



static void draw_wins_bordercorners(struct Win * w, WINDOW * pad)
{
    mvwaddch(pad, w->_start.y - 1, w->_start.x - 1, '+');
    mvwaddch(pad, w->_start.y - 1, w->_start.x + w->frame.size.x, '+');
    mvwaddch(pad, w->_start.y + w->frame.size.y, w->_start.x - 1, '+');
    mvwaddch(pad,
             w->_start.y + w->frame.size.y, w->_start.x + w->frame.size.x, '+');
    if (0 != w->_next)
    {
        draw_wins_bordercorners(w->_next, pad);
    }
}



static void shift_win_forward(struct WinMeta * wmeta)
{
    if (wmeta->active == wmeta->_chain_end)
    {
        wmeta->_chain_end = wmeta->active->_prev;
        wmeta->_chain_end->_next = 0;
        wmeta->active->_next = wmeta->_chain_start;
        wmeta->active->_next->_prev = wmeta->active;
        wmeta->_chain_start = wmeta->active;
        wmeta->_chain_start->_prev = 0;
    }
    else
    {
        struct Win * old_prev = wmeta->active->_prev;
        struct Win * old_next = wmeta->active->_next;
        if (wmeta->_chain_end == wmeta->active->_next)
        {
            wmeta->_chain_end = wmeta->active;
            wmeta->active->_next = 0;
        }
        else
        {
            wmeta->active->_next = old_next->_next;
            wmeta->active->_next->_prev = wmeta->active;
        }
        if (wmeta->_chain_start == wmeta->active)
        {
            wmeta->_chain_start = old_next;
        }
        else
        {
            old_prev->_next = old_next;
        }
        old_next->_prev = old_prev;
        old_next->_next = wmeta->active;
        wmeta->active->_prev = old_next;
    }
}



static void shift_win_backward(struct WinMeta * wmeta)
{
    if (wmeta->active == wmeta->_chain_start)
    {
        wmeta->_chain_start = wmeta->active->_next;
        wmeta->_chain_start->_prev = 0;
        wmeta->active->_prev = wmeta->_chain_end;
        wmeta->active->_prev->_next = wmeta->active;
        wmeta->_chain_end = wmeta->active;
        wmeta->_chain_end->_next = 0;
    }
    else
    {
        struct Win * old_prev = wmeta->active->_prev;
        struct Win * old_next = wmeta->active->_next;
        if (wmeta->_chain_start == wmeta->active->_prev)
        {
            wmeta->_chain_start = wmeta->active;
            wmeta->active->_prev = 0;
        }
        else
        {
            wmeta->active->_prev = old_prev->_prev;
            wmeta->active->_prev->_next = wmeta->active;
        }
        if (wmeta->_chain_end == wmeta->active)
        {
            wmeta->_chain_end = old_prev;
        }
        else
        {
            old_next->_prev = old_prev;
        }
        old_prev->_next = old_next;
        old_prev->_prev = wmeta->active;
        wmeta->active->_next = old_prev;
    }
}



extern uint8_t init_win_meta(WINDOW * screen, struct WinMeta ** wmp)
{
    struct WinMeta * wmeta = malloc(sizeof(struct WinMeta));
    wmeta->_screen             = screen;
    uint32_t maxy_test         = getmaxy(screen);
    uint32_t maxx_test         = getmaxx(screen);
    if (maxy_test > UINT16_MAX || maxx_test > UINT16_MAX)
    {
        return 2;
    }
    wmeta->padframe.size.y     = maxy_test;
    wmeta->padframe.size.x     = maxx_test;
    wmeta->_chain_start        = 0;
    wmeta->_chain_end          = 0;
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
    w->_prev             = 0;
    w->_next             = 0;
    w->frame.curses_win  = 0;
    w->_title             = malloc(strlen(title) + 1);
    if (NULL == w->_title)
    {
        return 1;
    }
    sprintf(w->_title, "%s", title);
    w->data              = data;
    w->_draw             = func;
    if      (0 < width)
    {
        w->frame.size.x = width;
    }
    else if (0 > width)
    {
        w->frame.size.x = wmeta->padframe.size.x + width;
    }
    else
    {
        w->frame.size.x = wmeta->padframe.size.x;
    }
    if      (0 < height && height <= wmeta->padframe.size.y - 1)
    {
        w->frame.size.y = height;
    }
    else if (0 > height && wmeta->padframe.size.y + (height - 1) > 0)
    {
        w->frame.size.y = wmeta->padframe.size.y + (height - 1);
    }
    else
    {
        w->frame.size.y = wmeta->padframe.size.y - 1;
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
    if (0 != win->frame.curses_win)
    {
        delwin(win->frame.curses_win);
    }
    free(win->_title);
    free(win);
}



extern uint8_t append_win(struct WinMeta * wmeta, struct Win * w)
{
    if (0 != wmeta->_chain_start)
    {
        w->_prev = wmeta->_chain_end;
        wmeta->_chain_end->_next = w;
    }
    else
    {
        wmeta->active = w;
        wmeta->_chain_start = w;
    }
    wmeta->_chain_end = w;
    return update_wins(wmeta, w);
}



extern uint8_t suspend_win(struct WinMeta * wmeta, struct Win * w)
{
    destroy_win(w);

    if (wmeta->_chain_start != w)
    {
        w->_prev->_next = w->_next;
    }
    else
    {
        wmeta->_chain_start = w->_next;
    }
    char pad_refitted = 0;
    if (wmeta->_chain_end != w)
    {
        w->_next->_prev = w->_prev;
        if (wmeta->active == w)
        {
            wmeta->active = w->_next;
        }
        uint8_t test = update_wins(wmeta, w->_next);/* Positioning of         */
        if (0 != test)                              /* successor windows may  */
        {                                           /* be affected / need     */
            return test;                            /* correction. Note that  */
        }                                           /* update_wins() already  */
        pad_refitted = 1;                           /* refits the pad, voiding*/
    }                                               /* later need for that.   */
    else
    {
        wmeta->_chain_end = w->_prev;
        if (wmeta->active == w)
        {
            wmeta->active = w->_prev;
        }
    }

    w->_prev = 0;
    w->_next = 0;

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
            if (wmeta->active->_next != 0)
            {
                wmeta->active = wmeta->active->_next;
            }
            else
            {
                wmeta->active = wmeta->_chain_start;
            }
        }
        else
        {
            if (wmeta->active->_prev != 0)
            {
                wmeta->active = wmeta->active->_prev;
            }
            else
            {
                wmeta->active = wmeta->_chain_end;
            }
        }
    }
}



extern uint8_t shift_active_win(struct WinMeta * wmeta, char dir)
{
    if (   0 == wmeta->active                        /* No shifting with < 2  */
        || wmeta->_chain_start == wmeta->_chain_end) /* windows visible.      */
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
    return update_wins(wmeta, wmeta->_chain_start);
}



extern uint8_t draw_all_wins(struct WinMeta * wmeta)
{
    /* Empty everything before filling it a-new. */
    erase();
    wnoutrefresh(wmeta->_screen);
    werase(wmeta->padframe.curses_win);
    if (wmeta->_chain_start)
    {

        /* Draw windows' contents first, then their borders. */
        draw_wins(wmeta->_chain_start);
        draw_wins_borderlines(wmeta->_chain_start, wmeta->active,
                              wmeta->padframe.curses_win);
        draw_wins_bordercorners(wmeta->_chain_start,wmeta->padframe.curses_win);

        /* Draw virtual screen scroll hints. */
        if (wmeta->pad_offset > 0)
        {
            if (draw_scroll_hint(&wmeta->padframe,
                                 wmeta->pad_offset, wmeta->pad_offset + 1, '<'))
            {
                return 1;
            }
        }
        if (wmeta->pad_offset + wmeta->padframe.size.x
            < getmaxx(wmeta->padframe.curses_win) - 1)
        {
            if (draw_scroll_hint(&wmeta->padframe,
                                 wmeta->pad_offset + wmeta->padframe.size.x - 1,
                                 getmaxx(wmeta->padframe.curses_win)
                                 - (wmeta->pad_offset + wmeta->padframe.size.x),
                                 '>'))
            {
                return 1;
            }
        }

        /* Write virtual screen segment to be shown on physical screen into */
        /* ncurses screen buffer. */
        pnoutrefresh(wmeta->padframe.curses_win, 0, wmeta->pad_offset, 0, 0,
                     wmeta->padframe.size.y, wmeta->padframe.size.x-1);
    }

    /* Only at the end write accumulated changes to the physical screen. */
    doupdate();
    return 0;
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
