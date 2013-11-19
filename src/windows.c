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
#include "misc.h"      /* for center_offset(), try_malloc() */
#include "main.h"      /* for world global */
#include "rexit.h"     /* for exit_err() */



/* Fit virtual screen's width to minimum width demanded by current windows'
 * geometries.
 */
static void refit_pad();

/* Update geometry (sizes, positions) of window "w" and its successors in the
 * window chain. For the positioning algorithm place_win() is used.
 */
static void update_wins(struct Win * w);
static void place_win(struct Win * w);

/* Draw scroll hint (a line saying that there are "dist" more elements of
 * "unit" further into the direction symbolized by the "dir" char) into virtual
 * screen pad, onto an appropriate edge of either a window or the screen; the
 * left or right edge if "dir" is "<" or ">", or the upper or lower edge if it
 * is "^" or "v". "start" should be either the start coordinate of a window's
 * frame or .y=0, .x=wm->pad_offset if it describes the virtual screen pad.
 * winscroll_hint() and padscroll_hint() are wrappers to simplify these uses.
 */
static void scroll_hint(struct yx_uint16 fsize, char dir, uint16_t dist,
                        char * unit, struct yx_uint16 start);
static void winscroll_hint(struct Win * w, char dir, uint16_t dist);
static void padscroll_hint(char dir, uint16_t dist);

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
static void shift_win_forward();
static void shift_win_backward();



static void refit_pad()
{
    /* Determine rightmost window column. */
    uint32_t lastwcol = 0;
    struct Win * wp = world.wmeta->chain_start;
    while (wp != 0)
    {
        if ((uint32_t) wp->start.x + (uint32_t) wp->framesize.x > lastwcol + 1)
        {
            lastwcol = (uint32_t) wp->start.x + (uint32_t) wp->framesize.x - 1;
        }
        wp = wp->next;
    }

    /* Only resize the pad if the rightmost window column has changed. */
    char * err_s = "refit_pad() extends virtual screen beyond legal sizes.";
    char * err_m = "refit_pad() triggers memory alloc error via wresize().";
    if (getmaxx(world.wmeta->pad) + 1 != lastwcol)
    {
        uint8_t t = (lastwcol + 2 > UINT16_MAX);
        exit_err(t, err_s);
        t = wresize(world.wmeta->pad, getmaxy(world.wmeta->pad), lastwcol + 2);
        exit_err(t, err_m);
    }
}



static void update_wins(struct Win * w)
{
    place_win(w);
    refit_pad();
    if (0 != w->next)
    {
        update_wins(w->next);
    }
}



static void place_win(struct Win * w)
{
    /* If w is first window, it goes into the upper-left corner. */
    w->start.x = 0;
    w->start.y = 1;                             /* Leave space for title bar. */
    if (0 != w->prev)
    {

        /* If w is not first winddow, fit its top left corner to the top right
         * corner of w_top, the last predecessor window starting at the top.
         */
        struct Win * w_top = w->prev;
        while (w_top->start.y != 1)
        {
            w_top = w_top->prev;
        }
        w->start.x = w_top->start.x + w_top->framesize.x + 1;

        /* Fit w's top left corner to bottom left corner of its immediate
         * predecessor window if enough empty space is found there.
         */
        uint16_t w_prev_maxy = w->prev->start.y + w->prev->framesize.y;
        if (   w->framesize.x <= w->prev->framesize.x
            && w->framesize.y <  world.wmeta->padsize.y - w_prev_maxy)
        {
            w->start.x = w->prev->start.x;
            w->start.y = w_prev_maxy + 1;
        }

        /* Failing that, try to fit the top left corner to the top right corner
         * of the last predecessor w_test 1) not followed by top windows or any
         * other windows with a left corner further rightwards than its own left
         * corner 2) with enough space rightwards for w until the bottom right
         * corner of w_thro directly throning over it 3) and with this same
         * space extending for enough to the bottom for fitting in w.
         */
        else
        {
            struct Win * w_test = w->prev;
            struct Win * w_thro;
            while (w_test != w_top)
            {
                w_thro = w_test->prev;
                while (w_test->start.y <= w_thro->start.y)
                {
                    w_thro = w_thro->prev;
                }
                uint16_t w_thro_bottom = w_thro->start.y + w_thro->framesize.y;
                uint16_t free_width = (w_thro->start.x + w_thro->framesize.x)
                                      - (w_test->start.x + w_test->framesize.x);
                if (   w->framesize.y < world.wmeta->padsize.y - w_thro_bottom
                    && w->framesize.x < free_width)
                {
                    w->start.x = w_test->start.x + w_test->framesize.x + 1;
                    w->start.y = w_thro_bottom + 1;
                    break;
                }
                w_test = w_thro;
            }
        }
    }
}



static void scroll_hint(struct yx_uint16 fsize, char dir, uint16_t dist,
                        char * unit, struct yx_uint16 start)
{
    /* Decide on alignment (vertical/horizontal?), thereby hint text space. */
    char * more = "more";
    uint16_t dsc_space = fsize.x;
    if ('<' == dir || '>' == dir)
    {
        dsc_space = fsize.y;
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
        draw_offset = fsize.x - 1;
    }
    else if ('v' == dir)
    {
        draw_offset = fsize.y - 1;
    }
    uint16_t q = 0;
    for (; q < dsc_space; q++)
    {
        chtype c = dir | A_REVERSE;
        if (q >= dsc_offset && q < strlen(scrolldsc) + dsc_offset)
        {
            c = scrolldsc[q - dsc_offset] | A_REVERSE;
        }
        if ('<' == dir || '>' == dir)
        {
            mvwaddch(world.wmeta->pad, start.y + q, start.x + draw_offset, c);
        }
        else
        {
            mvwaddch(world.wmeta->pad, start.y + draw_offset, start.x + q, c);
        }
    }
}


static void padscroll_hint(char dir, uint16_t dist)
{
    struct yx_uint16 start;
    start.y = 0;
    start.x = world.wmeta->pad_offset;
    scroll_hint(world.wmeta->padsize, dir, dist, "columns", start);
}



static void winscroll_hint(struct Win * w, char dir, uint16_t dist)
{
    char * unit = "lines";
    if ('<' == dir || '>' == dir)
    {
        unit = "columns";
    }
    struct yx_uint16 start = w->start;
    scroll_hint(w->framesize, dir, dist, unit, start);
}



static void draw_wins(struct Win * w)
{
    w->draw(w);
    uint16_t y, x, size_y, size_x;
    size_y = w->winmapsize.y;
    size_x = w->winmapsize.x;
    uint16_t offset_y = center_offset(w->center.y, size_y, w->framesize.y);
    uint16_t offset_x = center_offset(w->center.x, size_x, w->framesize.x);
    for (y = offset_y; y < w->framesize.y + offset_y && y < size_y; y++)
    {
        for (x = offset_x; x < w->framesize.x + offset_x && x < size_x; x++)
        {
            chtype ch = w->winmap[(y * w->winmapsize.x) + x];
            mvwaddch(world.wmeta->pad, w->start.y + (y - offset_y),
                                       w->start.x + (x - offset_x), ch);
        }
    }
    free(w->winmap);
    w->winmap = NULL;
    w->winmapsize.y = 0;
    w->winmapsize.x = 0;
    if (offset_y > 0)
    {
        winscroll_hint(w, '^', offset_y + 1);
    }
    if (size_y > offset_y + w->framesize.y)
    {
        winscroll_hint(w, 'v', size_y - ((offset_y + w->framesize.y) - 1));
    }
    if (offset_x > 0)
    {
        winscroll_hint(w, '<', offset_x + 1);
    }
    if (size_x > offset_x + w->framesize.x)
    {
        winscroll_hint(w, '>', size_x - ((offset_x + w->framesize.x) - 1));
    }
    if (0 != w->next)
    {
        return draw_wins(w->next);
    }
}



static void draw_win_borderlines(struct Win * w, char active, WINDOW * pad)
{
    /* Draw vertical and horizontal border lines. */
    uint16_t y, x;
    for (y = w->start.y; y <= w->start.y + w->framesize.y; y++)
    {
        mvwaddch(pad, y, w->start.x - 1,              '|');
        mvwaddch(pad, y, w->start.x + w->framesize.x, '|');
    }
    for (x = w->start.x; x <= w->start.x + w->framesize.x; x++)
    {
        mvwaddch(pad, w->start.y - 1,              x, '-');
        mvwaddch(pad, w->start.y + w->framesize.y, x, '-');
    }

    /* Draw as much as possible of the title into center of top border line. */
    char min_title_length_visible = 3;  /* min. 1 char + 2 padding/decoration */
    if (w->framesize.x >= min_title_length_visible)
    {
        uint16_t title_offset = 0;
        if (w->framesize.x > strlen(w->title) + 2)
        {
            title_offset = (w->framesize.x - (strlen(w->title) + 2)) / 2;
        }                                    /* +2 is for padding/decoration */
        uint16_t length_visible = strnlen(w->title, w->framesize.x - 2);
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
    mvwaddch(pad, w->start.y - 1, w->start.x + w->framesize.x, '+');
    mvwaddch(pad, w->start.y + w->framesize.y, w->start.x - 1, '+');
    mvwaddch(pad, w->start.y + w->framesize.y, w->start.x + w->framesize.x,'+');
    if (0 != w->next)
    {
        draw_wins_bordercorners(w->next, pad);
    }
}



static void shift_win_forward()
{
    if (world.wmeta->active == world.wmeta->chain_end)
    {
        world.wmeta->chain_end = world.wmeta->active->prev;
        world.wmeta->chain_end->next = 0;
        world.wmeta->active->next = world.wmeta->chain_start;
        world.wmeta->active->next->prev = world.wmeta->active;
        world.wmeta->chain_start = world.wmeta->active;
        world.wmeta->chain_start->prev = 0;
    }
    else
    {
        struct Win * old_prev = world.wmeta->active->prev;
        struct Win * old_next = world.wmeta->active->next;
        if (world.wmeta->chain_end == world.wmeta->active->next)
        {
            world.wmeta->chain_end = world.wmeta->active;
            world.wmeta->active->next = 0;
        }
        else
        {
            world.wmeta->active->next = old_next->next;
            world.wmeta->active->next->prev = world.wmeta->active;
        }
        if (world.wmeta->chain_start == world.wmeta->active)
        {
            world.wmeta->chain_start = old_next;
        }
        else
        {
            old_prev->next = old_next;
        }
        old_next->prev = old_prev;
        old_next->next = world.wmeta->active;
        world.wmeta->active->prev = old_next;
    }
}



static void shift_win_backward()
{
    if (world.wmeta->active == world.wmeta->chain_start)
    {
        world.wmeta->chain_start = world.wmeta->active->next;
        world.wmeta->chain_start->prev = 0;
        world.wmeta->active->prev = world.wmeta->chain_end;
        world.wmeta->active->prev->next = world.wmeta->active;
        world.wmeta->chain_end = world.wmeta->active;
        world.wmeta->chain_end->next = 0;
    }
    else
    {
        struct Win * old_prev = world.wmeta->active->prev;
        struct Win * old_next = world.wmeta->active->next;
        if (world.wmeta->chain_start == world.wmeta->active->prev)
        {
            world.wmeta->chain_start = world.wmeta->active;
            world.wmeta->active->prev = 0;
        }
        else
        {
            world.wmeta->active->prev = old_prev->prev;
            world.wmeta->active->prev->next = world.wmeta->active;
        }
        if (world.wmeta->chain_end == world.wmeta->active)
        {
            world.wmeta->chain_end = old_prev;
        }
        else
        {
            old_next->prev = old_prev;
        }
        old_prev->next = old_next;
        old_prev->prev = world.wmeta->active;
        world.wmeta->active->next = old_prev;
    }
}



extern void init_win_meta(WINDOW * screen)
{
    char * f_name = "init_win_meta()";
    char * err_s = "init_win_meta() creates virtual screen beyond legal size.";
    char * err_m = "init_win_meta() triggers memory alloc error via newpad().";
    world.wmeta         = try_malloc(sizeof(struct WinMeta), f_name);
    world.wmeta->screen = screen;
    uint32_t maxy_test  = getmaxy(screen);
    uint32_t maxx_test  = getmaxx(screen);
    uint8_t test = (maxy_test > UINT16_MAX || maxx_test > UINT16_MAX);
    exit_err(test, err_s);
    world.wmeta->padsize.y   = maxy_test;
    world.wmeta->padsize.x   = maxx_test;
    world.wmeta->chain_start = 0;
    world.wmeta->chain_end   = 0;
    world.wmeta->pad_offset  = 0;
    world.wmeta->pad         = newpad(world.wmeta->padsize.y, 1);
    exit_err(NULL == world.wmeta->pad, err_m);
    world.wmeta->active      = 0;
}



extern void init_win(struct Win ** wp, char * title, int16_t height,
                     int16_t width, void * func)
{
    char * f_name = "init_win()";
    struct Win * w  = try_malloc(sizeof(struct Win), f_name);
    w->prev         = 0;
    w->next         = 0;
    w->winmapsize.y = 0;
    w->winmapsize.x = 0;
    w->winmap = NULL;
    w->title        = try_malloc(strlen(title) + 1, f_name);
    sprintf(w->title, "%s", title);
    w->draw         = func;
    w->center.y     = 0;
    w->center.x     = 0;
    if      (0 < width)
    {
        w->framesize.x = width;
    }
    else if (0 >= width)
    {
        w->framesize.x = world.wmeta->padsize.x + width;
    }
    if      (0 < height && height <= world.wmeta->padsize.y - 1)
    {
        w->framesize.y = height;
    }
    else if (0 >= height && world.wmeta->padsize.y + (height - 1) > 0)
    {
        w->framesize.y = world.wmeta->padsize.y + (height - 1);
    }
    *wp = w;
}



extern void free_winmeta()
{
    delwin(world.wmeta->pad);
    free(world.wmeta);
}



extern void free_win(struct Win * win)
{
    free(win->title);
    free(win);
}



extern void append_win(struct Win * w)
{
    if (0 != world.wmeta->chain_start)
    {
        w->prev = world.wmeta->chain_end;
        world.wmeta->chain_end->next = w;
    }
    else
    {
        world.wmeta->active = w;
        world.wmeta->chain_start = w;
    }
    world.wmeta->chain_end = w;
    update_wins(w);
}



extern void suspend_win(struct Win * w)
{
    if (world.wmeta->chain_start != w)
    {
        w->prev->next = w->next;
    }
    else
    {
        world.wmeta->chain_start = w->next;
    }
    char pad_refitted = 0;
    if (world.wmeta->chain_end != w)
    {
        w->next->prev = w->prev;
        if (world.wmeta->active == w)
        {
            world.wmeta->active = w->next;
        }
        update_wins(w->next);      /* Positioning of successor windows may be */
        pad_refitted = 1;          /* affected / need correction. Note that   */
    }                              /* update_wins() already refits the pad,   */
    else                           /* voiding later need for that.            */
    {
        world.wmeta->chain_end = w->prev;
        if (world.wmeta->active == w)
        {
            world.wmeta->active = w->prev;
        }
    }

    w->prev = 0;
    w->next = 0;

    if (0 == pad_refitted)
    {
        refit_pad();
    }
}



extern void reset_pad_offset(uint16_t new_offset)
{
    if (new_offset >= 0
        && (new_offset < world.wmeta->pad_offset
            || new_offset + world.wmeta->padsize.x < getmaxx(world.wmeta->pad)))
    {
        world.wmeta->pad_offset = new_offset;
    }
}



extern void resize_active_win(struct yx_uint16 size)
{
    if (0 != world.wmeta->active
        && size.x > 0 && size.y > 0 && size.y < world.wmeta->padsize.y)
    {
        world.wmeta->active->framesize = size;
        update_wins(world.wmeta->active);         /* Positioning of following */
    }                                             /* windows may be affected. */
}



extern void cycle_active_win(char dir)
{
    if (0 != world.wmeta->active)
    {
        if ('f' == dir)
        {
            if (world.wmeta->active->next != 0)
            {
                world.wmeta->active = world.wmeta->active->next;
            }
            else
            {
                world.wmeta->active = world.wmeta->chain_start;
            }
        }
        else
        {
            if (world.wmeta->active->prev != 0)
            {
                world.wmeta->active = world.wmeta->active->prev;
            }
            else
            {
                world.wmeta->active = world.wmeta->chain_end;
            }
        }
    }
}



extern void shift_active_win(char dir)
{
    if (   0 == world.wmeta->active  /* No shifting with < 2 windows visible. */
        || world.wmeta->chain_start == world.wmeta->chain_end)
    {
        return;
    }
    if ('f' == dir)
    {
        shift_win_forward();
    }
    else
    {
        shift_win_backward();
    }
    update_wins(world.wmeta->chain_start);
}



extern void draw_all_wins()
{
    /* Empty everything before filling it a-new. */
    erase();
    wnoutrefresh(world.wmeta->screen);
    werase(world.wmeta->pad);
    if (world.wmeta->chain_start)
    {

        /* Draw windows' borders first, then windows. */
        draw_wins_borderlines(world.wmeta->chain_start, world.wmeta->active,
                              world.wmeta->pad);
        draw_wins_bordercorners(world.wmeta->chain_start, world.wmeta->pad);
        draw_wins(world.wmeta->chain_start);

        /* Draw virtual screen scroll hints. */
        if (world.wmeta->pad_offset > 0)
        {
            padscroll_hint('<', world.wmeta->pad_offset + 1);
        }
        uint16_t size_x = getmaxx(world.wmeta->pad);
        uint16_t right_edge = world.wmeta->pad_offset + world.wmeta->padsize.x;
        if (right_edge < size_x - 1)
        {
            padscroll_hint('>', size_x - right_edge);
        }

        /* Write pad segment to be shown on physical screen to screen buffer. */
        pnoutrefresh(world.wmeta->pad, 0, world.wmeta->pad_offset, 0, 0,
                     world.wmeta->padsize.y, world.wmeta->padsize.x - 1);
    }

    /* Only at the end write accumulated changes to the physical screen. */
    doupdate();
}
