/* src/client/windows.c */

#include "windows.h"
#include <ncurses.h> /* chtype, getmaxx(), getmaxy(), erase(), werase(),
                      * endwin(), delwin(), wnoutrefresh(), pnoutrefresh(),
                      * doupdate(), refresh(), delwin(), newpad(), mvwaddch(),
                      * mvwaddstr(), wresize()
                      */
#include <stddef.h> /* NULL */
#include <stdlib.h> /* free(), atoi() */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <stdio.h> /* sprintf() */
#include <string.h> /* memcpy(), strlen(), strnlen(), strchr(), memset() */
#include "../common/rexit.h" /* exit_trouble(), exit_err() */
#include "../common/readwrite.h" /* try_fputc(), try_write(), try_fgets(),
                                  * try_fgetc()
                                  */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint16.h" /* struct yx_uint16 */
#include "draw_wins.h" /* draw_winconf_geometry(), draw_winconf_keybindings(),
                        * draw_win_inventory(), draw_win_info(), draw_win_log(),
                        * draw_win_available_keybindings(), draw_win_map(),
                        * draw_win_keybindings_winconf_keybindings(),
                        * draw_win_keybindings_winconf_geometry(),
                        * draw_win_keybindings_global()
                        */
#include "keybindings.h" /* free_keybindings(), write_keybidings_to_file(),
                          * read_keybindings_from_file()
                          */
#include "world.h" /* global world */



/* Get position of id "c" in world.winDB.order or return Win before/after (or
 * NULL if there is no window before/after).
 */
static uint8_t get_pos_in_order(char c);
static struct Win * get_win_after(char c);
static struct Win * get_win_before(char c);

/* Calculate "id"'s window's size from v_screen size and .target_(height/width).
 * A .target_width == 0 makes the window as wide as .t_screen. .target_height ==
 * 0 sets the maximum allowed height: one cell smaller than that of .v_screen.
 * Negative values make width/height so many cells smaller than what 0 would
 * set. Values that would reduce the window height or width to less than 1 cell
 * according to the aforementioned rules set the height/width as if they were 0.
 */
static void init_win_size_from_winconf_and_v_screen_size(char id);

/* Get (Win->draw) function identified by "c"; NULL if c not mapped to one.
 * match_func() is just a little helper to it.
 */
static uint8_t match_func(char c, void (** f) (), char c_m, void (* f_m) ());
static void * get_drawfunc_by_char(char c);

/* Write "win"'s size back to .target_(height/width) as per .target_*_type. */
static void set_win_target_size(struct Win * win);

/* Iterate over chars of world.winDB.ids array / string. Restart after \0.*/
static char get_next_win_id();

/* Make .v_screen just wide enough to contain all visible windows. */
static void refit_v_screen();

/* Update geometry (sizes, positions) of window "w" and its successors in the
 * window chain. Use place_win() for the positioning algorithm.
 */
static void update_wins(struct Win * w);
static void place_win(struct Win * w);

/* Draw scroll hint (a line saying that there are "dist" more elements of "unit"
 * further into the direction symbolized by "dir") into .v_screen, onto an
 * appropriate edge of a window or .v_screen; the left/right edge if "dir" is
 * "<"/">", or the top/bottom edge if it is "^"/"v". Use "start" as the start
 * coordinate of the frame that is to contain the scroll hints. winscroll_hint()
 * is a wrapper that uses the border of window "w" as this frame.
 */
static void scroll_hint(struct yx_uint16 fsize, char dir, uint16_t dist,
                        char * unit, struct yx_uint16 start);
static void winscroll_hint(struct Win * w, char dir, uint16_t dist);

/* draw_win_borderlines() draws vertical/horizontal borders of window "w" sans
 * corners into .v_screen. It draws the top border line as the windows' title
 * bar (highlighted if the window is selected as active). It is called
 * recursively by draw_wins_borderlines() on all windows from "w" on.
 * draw_wins_bordercorners() draws the border corners of "w" and its successors.
 */
static void draw_win_borderlines(struct Win * w);
static void draw_wins_borderlines(struct Win * w);
static void draw_wins_bordercorners(struct Win * w);

/* Draw contents of all windows in window chain from window "w" onwards. */
static void draw_wins(struct Win * w);

/* Append/suspend window "w" to/from chain of visible windows. Appended windows
 * will become active. Suspended active windows will move the active window
 * selection to their successor in the window chain or, failing that, their
 * predecessor, or, failing that, to 0 (no window active).
 */
static void append_win(struct Win * w);
static void suspend_win(struct Win * w);

/* Copy Win content pointed to by "win" into appendend world.winDB.wins area. */
static void add_win_to_winDB(struct Win * win);



static uint8_t get_pos_in_order(char c)
{
    uint8_t i;
    for (i = 0; c != world.winDB.order[i]; i++);
    return i;
}



static struct Win * get_win_after(char c)
{
    return get_win_by_id(world.winDB.order[get_pos_in_order(c) + 1]);
}



static struct Win * get_win_before(char c)
{
    uint8_t i = get_pos_in_order(c);
    if (i > 0)
    {
        return get_win_by_id(world.winDB.order[i - 1]);
    }
    return NULL;
}



static void init_win_size_from_winconf_and_v_screen_size(char id)
{
    struct Win * w = get_win_by_id(id);
    w->frame_size.y  = world.winDB.v_screen_size.y - 1;
    if      (   0 < w->target_height
             && w->target_height <= world.winDB.v_screen_size.y - 1)
    {
        w->frame_size.y = w->target_height;
    }
    else if (   0 > w->target_height
             && world.winDB.v_screen_size.y + (w->target_height - 1) > 0)
    {
        w->frame_size.y = world.winDB.v_screen_size.y + (w->target_height - 1);
    }
    w->frame_size.x  = world.winDB.v_screen_size.x;
    if      (0 < w->target_width)
    {
        w->frame_size.x = w->target_width;
    }
    else if (   0 > w->target_width
             && world.winDB.v_screen_size.x + w->target_width > 0)
    {
        w->frame_size.x = world.winDB.v_screen_size.x + w->target_width;
    }
}



static uint8_t match_func(char c, void (** f) (), char c_m, void (* f_m) ())
{
    if (c == c_m)
    {
        * f = f_m;
        return 1;
    }
    return 0;
}



static void * get_drawfunc_by_char(char c)
{
    void (* f) (struct Win *) = NULL;
    if (   match_func(c, &f, 'c', draw_win_inventory)
        || match_func(c, &f, 'i', draw_win_info)
        || match_func(c, &f, 'l', draw_win_log)
        || match_func(c, &f, 'k', draw_win_available_keybindings)
        || match_func(c, &f, 'm', draw_win_map)
        || match_func(c, &f, '0', draw_win_keybindings_global)
        || match_func(c, &f, '1', draw_win_keybindings_winconf_geometry)
        || match_func(c, &f, '2', draw_win_keybindings_winconf_keybindings));
    return f;
}



static void set_win_target_size(struct Win * wcp)
{
    if      (0 == wcp->target_height_type)
    {
        wcp->target_height = wcp->frame_size.y;
    }
    else if (1 == wcp->target_height_type)
    {
        wcp->target_height = wcp->frame_size.y - world.winDB.v_screen_size.y +1;
    }
    if      (0 == wcp->target_width_type)
    {
        wcp->target_width = wcp->frame_size.x;
    }
    else if (1 == wcp->target_width_type)
    {
        wcp->target_width = wcp->frame_size.x - world.winDB.v_screen_size.x;
    }
}



static char get_next_win_id()
{
    static uint8_t i = 0;
    char c = world.winDB.ids[i];
    if (0 == c)
    {
        i = 0;
        return c;
    }
    i++;
    return c;
}



static void refit_v_screen()
{
    /* Determine rightmost window column. */
    uint32_t lastwcol = 0;
    struct Win * wp = get_win_by_id(world.winDB.order[0]);
    while (wp != 0)
    {
        if ((uint32_t) wp->start.x + (uint32_t) wp->frame_size.x > lastwcol + 1)
        {
            lastwcol = (uint32_t) wp->start.x + (uint32_t) wp->frame_size.x - 1;
        }
        wp = get_win_after(wp->id);
    }

    /* Only resize .v_screen if the rightmost window column has changed. */
    char * err_s = "refit_v_screen() grows virtual screen beyond legal sizes.";
    char * err_m = "refit_v_screen() triggers memory alloc error in wresize().";
    if (getmaxx(world.winDB.v_screen) + 1 != lastwcol)
    {
        uint8_t t = (lastwcol + 2 > UINT16_MAX);
        exit_err(t, err_s);
        t = wresize(world.winDB.v_screen, getmaxy(world.winDB.v_screen),
                    lastwcol + 2);
        exit_err(t, err_m);
    }
}



static void update_wins(struct Win * w)
{
    place_win(w);
    refit_v_screen();
    struct Win * next = get_win_after(w->id);
    if (next)
    {
        update_wins(next);
    }
}



static void place_win(struct Win * w)
{
    /* If w is first window, it goes into the top left corner. */
    w->start.x = 0;
    w->start.y = 1;                             /* Leave space for title bar. */
    struct Win * w_prev = get_win_before(w->id);
    if (w_prev)
    {

        /* If not, fit w's top left to top right of last top predecessor. */
        struct Win * w_top = w_prev;
        for (;
             w_top->start.y != 1;
             w_top = get_win_before(w_top->id));
        w->start.x = w_top->start.x + w_top->frame_size.x + 1;

        /* Fit w's top left to bottom left of its ->prev if enough space. */
        uint16_t w_prev_maxy = w_prev->start.y + w_prev->frame_size.y;
        if (   w->frame_size.x <= w_prev->frame_size.x
            && w->frame_size.y <  world.winDB.v_screen_size.y - w_prev_maxy)
        {
            w->start.x = w_prev->start.x;
            w->start.y = w_prev_maxy + 1;
            return;
        }

        /* Failing that, try to fit w' top left to the top right of the last
         * predecessor w_test 1) not followed by windows with a left corner
         * further rightwards than its own 2) with enough space rightwards for w
         * until the bottom right of w_thr directly throning over it 3) and with
         * this same space extending far enough to the bottom for fitting in w.
         */
        struct Win * w_test = w_prev;
        struct Win * w_thr;
        while (w_test != w_top)
        {
            for (w_thr = get_win_before(w_test->id);
                 w_test->start.y <= w_thr->start.y;
                 w_thr = get_win_before(w_thr->id));
            uint16_t w_thr_bottom = w_thr->start.y + w_thr->frame_size.y;
            uint16_t free_width =   (w_thr->start.x + w_thr->frame_size.x)
                                  - (w_test->start.x + w_test->frame_size.x);
            if (   w->frame_size.y < world.winDB.v_screen_size.y - w_thr_bottom
                && w->frame_size.x < free_width)
            {
                w->start.x = w_test->start.x + w_test->frame_size.x + 1;
                w->start.y = w_thr_bottom + 1;
                break;
            }
            w_test = w_thr;
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
            mvwaddch(world.winDB.v_screen, start.y+q, start.x+draw_offset, c);
            continue;
        }
        mvwaddch(world.winDB.v_screen, start.y + draw_offset, start.x + q, c);
    }
}



static void winscroll_hint(struct Win * w, char dir, uint16_t dist)
{
    char * unit = "lines";
    if ('<' == dir || '>' == dir)
    {
        unit = "columns";
    }
    struct yx_uint16 start = w->start;
    scroll_hint(w->frame_size, dir, dist, unit, start);
}



static void draw_win_borderlines(struct Win * w)
{
    /* Draw vertical and horizontal border lines. */
    uint16_t y, x;
    for (y = w->start.y; y <= w->start.y + w->frame_size.y; y++)
    {
        mvwaddch(world.winDB.v_screen, y, w->start.x - 1,              '|');
        mvwaddch(world.winDB.v_screen, y, w->start.x + w->frame_size.x, '|');
    }
    for (x = w->start.x; x <= w->start.x + w->frame_size.x; x++)
    {
        mvwaddch(world.winDB.v_screen, w->start.y - 1,              x, '-');
        mvwaddch(world.winDB.v_screen, w->start.y + w->frame_size.y, x, '-');
    }

    /* Draw as much as possible of the title into center of top border line. */
    uint8_t min_title_length_visible = 3;/* min. 1 char +2 padding/decoration*/
    if (w->frame_size.x >= min_title_length_visible)
    {
        uint16_t offset = 0;
        if (w->frame_size.x > strlen(w->title) + 2)
        {
            offset = (w->frame_size.x - (strlen(w->title) + 2)) / 2;
        }                                    /* +2 is for padding/decoration */
        uint16_t length_visible = strnlen(w->title, w->frame_size.x - 2);
        char title[length_visible + 3];
        char decoration = ' ';
        if (w->id == world.winDB.active)
        {
            decoration = '$';
        }
        memcpy(title + 1, w->title, length_visible);
        title[0] = title[length_visible + 1] = decoration;
        title[length_visible + 2] = '\0';
        mvwaddstr(world.winDB.v_screen, w->start.y-1, w->start.x+offset, title);
    }
}



static void draw_wins_borderlines(struct Win * w)
{
    draw_win_borderlines(w);
    struct Win * next = get_win_after(w->id);
    if (next)
    {
        draw_wins_borderlines(next);
    }
}



static void draw_wins_bordercorners(struct Win * w)
{
    mvwaddch(world.winDB.v_screen,
             w->start.y - 1, w->start.x - 1,              '+');
    mvwaddch(world.winDB.v_screen,
             w->start.y - 1, w->start.x + w->frame_size.x, '+');
    mvwaddch(world.winDB.v_screen,
             w->start.y + w->frame_size.y, w->start.x - 1, '+');
    mvwaddch(world.winDB.v_screen, w->start.y + w->frame_size.y,
             w->start.x + w->frame_size.x,                 '+');
    struct Win * next = get_win_after(w->id);
    if (next)
    {
        draw_wins_bordercorners(next);
    }
}



static void draw_wins(struct Win * w)
{
    void (* drawfunc) (struct Win *) = get_drawfunc_by_char(w->id);
    if      (1 == w->view)
    {
        drawfunc = draw_winconf_geometry;
    }
    else if (2 == w->view)
    {
        drawfunc = draw_winconf_keybindings;
    }
    drawfunc(w);
    uint16_t size_y = w->winmap_size.y;
    uint16_t size_x = w->winmap_size.x;
    uint16_t offset_y = center_offset(w->center.y, size_y, w->frame_size.y);
    uint16_t offset_x = center_offset(w->center.x, size_x, w->frame_size.x);
    uint16_t y, x;
    for (y = offset_y; y < w->frame_size.y + offset_y && y < size_y; y++)
    {
        for (x = offset_x; x < w->frame_size.x + offset_x && x < size_x; x++)
        {
            chtype ch = w->winmap[(y * w->winmap_size.x) + x];
            mvwaddch(world.winDB.v_screen, w->start.y + (y - offset_y),
                                      w->start.x + (x - offset_x), ch);
        }
    }
    free(w->winmap); /* NULL so draw_wins.c's try_resize_winmap() may always  */
    w->winmap = NULL;/* free() it before (re-)allocation, even the first time.*/
    memset(&w->winmap_size, 0, sizeof(struct yx_uint16));
    if (offset_y > 0)
    {
        winscroll_hint(w, '^', offset_y + 1);
    }
    if (size_y > offset_y + w->frame_size.y)
    {
        winscroll_hint(w, 'v', size_y - ((offset_y + w->frame_size.y) - 1));
    }
    if (offset_x > 0)
    {
        winscroll_hint(w, '<', offset_x + 1);
    }
    if (size_x > offset_x + w->frame_size.x)
    {
        winscroll_hint(w, '>', size_x - ((offset_x + w->frame_size.x) - 1));
    }
    struct Win * next = get_win_after(w->id);
    if (next)
    {
        return draw_wins(next);
    }
}



static void append_win(struct Win * w)
{
    char * f_name = "append_win()";
    uint8_t old_size = strlen(world.winDB.order) + 1;
    char * new_order = try_malloc(old_size + 1, f_name);
    memcpy(new_order, world.winDB.order, old_size - 1);
    new_order[old_size - 1] = w->id;
    new_order[old_size] = '\0';
    free(world.winDB.order);
    world.winDB.order = new_order;
    world.winDB.active = w->id;
    update_wins(w);
}



static void suspend_win(struct Win * w)
{
    char * f_name = "suspend_win()";
    uint8_t new_size = strlen(world.winDB.order);
    char * new_order = try_malloc(new_size, f_name);
    uint8_t i = get_pos_in_order(w->id);
    char next_char = world.winDB.order[i + 1];
    world.winDB.order[i] = '\0';
    char * second_part = &world.winDB.order[i + 1];
    sprintf(new_order, "%s%s", world.winDB.order, second_part);
    free(world.winDB.order);
    world.winDB.order = new_order;
    world.winDB.active = world.winDB.order[i];
    if (!world.winDB.order[i] && 0 < i)
    {
        world.winDB.active = world.winDB.order[i - 1];
    }
    if (world.winDB.order[i])
    {
        update_wins(get_win_by_id(next_char)); /* Already calls               */
        return;                                /* refit_v_screen(), so leave. */
    }
    refit_v_screen();
}



static void add_win_to_winDB(struct Win * win)
{
    char * f_name = "add_win_to_winDB()";
    if (world.winDB.ids)
    {
        uint8_t old_ids_size = strlen(world.winDB.ids);
        char * new_ids = try_malloc(old_ids_size + 1 + 1, f_name);
        sprintf(new_ids, "%s%c", world.winDB.ids, win->id);
        free(world.winDB.ids);
        world.winDB.ids = new_ids;
        uint16_t old_wins_size = old_ids_size * sizeof(struct Win);
        uint16_t new_wins_size = old_wins_size + sizeof(struct Win);
        struct Win * new_wins = try_malloc(new_wins_size, f_name);
        memcpy(new_wins, world.winDB.wins, old_wins_size);
        new_wins[old_ids_size] = *win;
        free(world.winDB.wins);
        world.winDB.wins = new_wins;
        return;
    }
    world.winDB.ids = try_malloc(2, f_name);
    sprintf(world.winDB.ids, "%c", win->id);
    world.winDB.wins = try_malloc(sizeof(struct Win), f_name);
    world.winDB.wins[0] = *win;
}



extern uint16_t center_offset(uint16_t position, uint16_t mapsize,
                              uint16_t frame_size)
{
    uint16_t offset = 0;
    if (mapsize > frame_size)
    {
        if (position > frame_size / 2)
        {
            if (position < mapsize - (frame_size / 2))
            {
                offset = position - (frame_size / 2);
            }
            else
            {
                offset = mapsize - frame_size;
            }
        }
    }
    return offset;
}



extern struct Win * get_win_by_id(char id)
{
    uint8_t i = 0;
    while ('\0' != world.winDB.ids[i])
    {
        if (id == world.winDB.ids[i])
        {
            return &world.winDB.wins[i];
        }
        i++;
    }
    return NULL;
}



extern uint8_t read_winconf_from_file(char * line, uint32_t linemax,
                                      FILE * file)
{
    char * f_name = "read_winconf_from_file()";
    int test = try_fgetc(file, f_name);
    if (EOF == test)
    {
        return 0;
    }
    struct Win win;
    memset(&win, 0, sizeof(struct Win));
    win.id = (char) test;
    try_fgetc(file, f_name);
    try_fgets(line, linemax + 1, file, f_name);
    win.title = try_malloc(strlen(line), f_name);
    memcpy(win.title, line, strlen(line) - 1);  /* Eliminate newline char */
    win.title[strlen(line) - 1] = '\0';         /* char at end of string. */
    try_fgets(line, linemax + 1, file, f_name);
    win.target_height = atoi(line);
    win.target_height_type = (0 >= win.target_height);
    try_fgets(line, linemax + 1, file, f_name);
    win.target_width = atoi(line);
    win.target_width_type = (0 >= win.target_width);
    read_keybindings_from_file(line, linemax, file, &win.kb);
    add_win_to_winDB(&win);
    return 1;
}



extern void write_winconf_of_id_to_file(FILE * file, char c, char * delim)
{
    char * f_name = "write_winconf_of_id_to_file()";
    struct Win * wc = get_win_by_id(c);
    uint8_t size = strlen(wc->title) + 2;
    if (size < 7)  /* Ensure that at least 5 + 2 char fit into line so that   */
    {              /* the digit representation of any uint16_t may be stored. */
        size = 7;
    }
    char line[size];
    sprintf(line, "%c\n", wc->id);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%s\n", wc->title);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%d\n", wc->target_height);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%d\n", wc->target_width);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    write_keybindings_to_file(file, &wc->kb, delim);
}



extern void read_order_wins_visible_active(char * line, uint32_t linemax,
                                           FILE * file)
{
    char * f_name = "read_order_wins_visible_active()";
    char win_order[linemax + 1];
    try_fgets(win_order, linemax + 1, file, f_name);
    win_order[strlen(win_order) - 1] = '\0';
    world.winDB.order = try_malloc(strlen(win_order) + 1, f_name);
    sprintf(world.winDB.order, "%s", win_order);
    int char_or_eof = try_fgetc(file, f_name);
    char * err_eof = "fgetc() unexpectedly hitting EOF";
    exit_trouble(EOF == char_or_eof, f_name, err_eof);
    world.winDB.active = (uint8_t) char_or_eof;
    exit_trouble(EOF == try_fgetc(file, f_name), f_name, err_eof);
    try_fgets(line, linemax + 1, file, f_name);
}



extern void write_order_wins_visible_active(FILE * file, char * delim)
{
    char * f_name = "write_order_wins_visible_active()";
    try_fwrite(world.winDB.order, strlen(world.winDB.order), 1, file, f_name);
    try_fputc('\n', file, f_name);
    try_fputc(world.winDB.active, file, f_name);
    try_fputc('\n', file, f_name);
    try_fwrite(delim, strlen(delim), 1, file, f_name);
}



extern void make_v_screen_and_init_win_sizes()
{
    char * f_name = "make_v_screen_and_init_win_sizes()";
    char * err_s = "creating an illegaly large virtual screen";
    char * err_m = "triggering a memory allocation error via newpad()";
    uint32_t maxy_test = getmaxy(world.winDB.t_screen);
    uint32_t maxx_test = getmaxx(world.winDB.t_screen);
    exit_trouble(maxy_test>UINT16_MAX || maxx_test>UINT16_MAX, f_name, err_s);
    world.winDB.v_screen_size.y = maxy_test;
    world.winDB.v_screen_size.x = maxx_test;
    world.winDB.v_screen = newpad(world.winDB.v_screen_size.y, 1);
    exit_trouble(NULL == world.winDB.v_screen, f_name, err_m);
    char id;
    while (0 != (id = get_next_win_id()))
    {
        init_win_size_from_winconf_and_v_screen_size(id);
    }
}



extern void free_winDB()
{
    char id;
    while (0 != (id = get_next_win_id()))
    {
        struct Win * wc = get_win_by_id(id);
        free(wc->title);
        free_keybindings(wc->kb.kbs);
    }
    free(world.winDB.ids);  /* NULL this too since add_win_to_winDB() checks  */
    world.winDB.ids = NULL; /* for it to detect its first post-DB-purge round.*/
    free(world.winDB.wins);
    free(world.winDB.order);
}



extern void winch_called(int signal)
{
    world.winch = 1;
}



extern void reset_windows_on_winch()
{
    endwin();  /* "[S]tandard way" to recalibrate ncurses post SIGWINCH, says */
    refresh(); /* <http://invisible-island.net/ncurses/ncurses-intro.html>.   */
    char tmp_order[strlen(world.winDB.order) + 1];
    sprintf(tmp_order, "%s", world.winDB.order);
    uint8_t i;
    char tmp_active = world.winDB.active;
    for (i = 0; i < strlen(tmp_order); toggle_window(tmp_order[i]), i++);
    delwin(world.winDB.v_screen);
    make_v_screen_and_init_win_sizes();
    for (i = 0; i < strlen(tmp_order); toggle_window(tmp_order[i]), i++);
    world.winDB.active = tmp_active;
}



extern void draw_all_wins()
{
    /* Empty everything before filling it a-new. */
    erase();
    wnoutrefresh(world.winDB.t_screen);
    werase(world.winDB.v_screen);
    if (world.winDB.active)
    {

        /* Draw borders, wins. Order matters: corners should overwrite lines. */
        draw_wins_borderlines(get_win_by_id(world.winDB.order[0]));
        draw_wins_bordercorners(get_win_by_id(world.winDB.order[0]));
        draw_wins(get_win_by_id(world.winDB.order[0]));

        /* Draw .v_screen scroll hints. */
        struct yx_uint16 start;
        start.y = 0;
        start.x = world.winDB.v_screen_offset;
        char * cols_string = "columns";
        if (world.winDB.v_screen_offset > 0)
        {
            scroll_hint(world.winDB.v_screen_size, '<',
                        world.winDB.v_screen_offset + 1, cols_string, start);
        }
        uint16_t size_x = getmaxx(world.winDB.v_screen);
        uint16_t right_edge =   world.winDB.v_screen_offset
                              + world.winDB.v_screen_size.x;
        if (right_edge < size_x - 1)
        {
            scroll_hint(world.winDB.v_screen_size, '>',
                        size_x - right_edge, cols_string, start);
        }

        /* Put .v_screen segment to be shown on .t_screen to .t_screen buffer.*/
        pnoutrefresh(world.winDB.v_screen, 0, world.winDB.v_screen_offset, 0, 0,
                     world.winDB.v_screen_size.y,
                     world.winDB.v_screen_size.x - 1);
    }

    /* Only at the end write accumulated changes to .t_screen. */
    doupdate();
}



extern void toggle_window(char id)
{
    struct Win * win = get_win_by_id(id);
    if (NULL == strchr(world.winDB.order, id))
    {
        append_win(win);
        return;
    }
    suspend_win(win);
}



extern void toggle_winconfig()
{
    if (!world.winDB.active)
    {
        return;
    }
    struct Win * w = get_win_by_id(world.winDB.active);
    if      (0 == w->view)
    {
        w->view          = 1;
        w->target_center = w->center;
        memset(&w->center, 0, sizeof(struct yx_uint16));
        return;
    }
    else if (1 == w->view)
    {
        w->view          = 2;
        w->center.x      = 0;
        return;
    }
    w->view          = 0;
    w->center        = w->target_center;
}



extern void toggle_win_size_type(char axis)
{
    struct Win * w = get_win_by_id(world.winDB.active);
    if ('y' == axis)
    {
        w->target_height_type = (0 == w->target_height_type);
        set_win_target_size(w);
        return;
    }
    w->target_width_type = (   0 == w->target_width_type
                            && w->frame_size.x <= world.winDB.v_screen_size.x);
    set_win_target_size(w);
}



extern void resize_active_win(char change)
{
    if (world.winDB.active)
    {
        struct Win * w = get_win_by_id(world.winDB.active);
        if      (change == '-' && w->frame_size.y > 1)
        {
            w->frame_size.y--;
        }
        else if (change == '_' && w->frame_size.y > 1)
        {
            w->frame_size.x--;
        }
        else if (   change == '+'
                 && w->frame_size.y < world.winDB.v_screen_size.y - 1)
        {
            w->frame_size.y++;
        }
        else if (change == '*' && w->frame_size.y < UINT16_MAX)
        {
            w->frame_size.x++;
        }
        if (   1 == w->target_width_type
            && w->frame_size.x > world.winDB.v_screen_size.x)
        {
            w->target_width_type = 0;
        }
        set_win_target_size(w);
        update_wins(w);
    }
}



extern void shift_active_win(char dir)
{
    uint8_t len_order = strlen(world.winDB.order);
    if (1 < len_order)
    {
        char tmp[len_order + 1];
        tmp[len_order] = '\0';
        uint8_t pos = get_pos_in_order(world.winDB.active);
        if ('f' == dir)
        {
            if (pos == len_order - 1)
            {
                memcpy(tmp + 1, world.winDB.order, len_order - 1);
                tmp[0] = world.winDB.active;
                memcpy(world.winDB.order, tmp, len_order + 1);
            }
            else
            {
                world.winDB.order[pos] = world.winDB.order[pos + 1];
                world.winDB.order[pos + 1] = world.winDB.active;
            }
        }
        else
        {
            if (pos == 0)
            {
                memcpy(tmp, world.winDB.order + 1, len_order - 1);
                tmp[len_order - 1] = world.winDB.active;
                memcpy(world.winDB.order, tmp, len_order + 1);
            }
            else
            {
                world.winDB.order[pos] = world.winDB.order[pos - 1];
                world.winDB.order[pos - 1] = world.winDB.active;
            }
        }
        update_wins(get_win_by_id(world.winDB.order[0]));
    }
}



extern void scroll_v_screen(char dir)
{
    if      (   '+' == dir
             &&   world.winDB.v_screen_offset + world.winDB.v_screen_size.x + 1
                < getmaxx(world.winDB.v_screen))
    {
        world.winDB.v_screen_offset++;
    }
    else if (   '-' == dir
             && world.winDB.v_screen_offset > 0)
    {
        world.winDB.v_screen_offset--;
    }
}



extern void cycle_active_win(char dir)
{
    uint8_t len_order = strlen(world.winDB.order);
    if (1 < len_order)
    {
        uint8_t pos = get_pos_in_order(world.winDB.active);
        if ('f' == dir)
        {
            world.winDB.active = world.winDB.order[pos + 1];
            if ('\0' == world.winDB.active)
            {
                world.winDB.active = world.winDB.order[0];
            }
            return;
        }
        if (pos > 0)
        {
            world.winDB.active = world.winDB.order[pos - 1];
            return;
        }
        world.winDB.active = world.winDB.order[len_order - 1];
    }
}
