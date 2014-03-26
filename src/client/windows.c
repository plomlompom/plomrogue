/* src/client/windows.c */

#define _POSIX_C_SOURCE 200809L /* strnlen() */
#include "windows.h"
#include <ncurses.h> /* chtype, getmaxx(), getmaxy(), erase(), werase(),
                      * endwin(), delwin(), wnoutrefresh(), pnoutrefresh(),
                      * doupdate(), refresh(), delwin(), newpad(), mvwaddch(),
                      * mvwaddstr()
                      */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* memcpy(), strlen(), strnlen(), strchr(), memset() */
#include "../common/err_try_fgets.h" /* err_try_fgets(), err_line() */
#include "../common/readwrite.h" /* try_fputc(), try_write(), try_fgetc() */
#include "../common/rexit.h" /* exit_trouble(), exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint16.h" /* struct yx_uint16 */
#include "array_append.h" /* array_append() */
#include "draw_wins.h" /* draw_winconf_geometry(), draw_winconf_keybindings(),
                        * draw_win_inventory(), draw_win_info(), draw_win_log(),
                        * draw_win_available_keybindings(), draw_win_map(),
                        * draw_win_keybindings_winconf_keybindings(),
                        * draw_win_keybindings_winconf_geometry(),
                        * draw_win_keybindings_global()
                        */
#include "keybindings.h" /* write_keybidings_to_file(),
                          * read_keybindings_from_file()
                          */
#include "wincontrol.h" /* toggle_window() */
#include "world.h" /* global world */



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
static void (* get_drawfunc_by_char(char c)) ();

/* Iterate over chars of world.winDB.ids array / string. Restart after \0.*/
static char get_next_win_id();

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



static void (* get_drawfunc_by_char(char c)) ()
{
    void (* f) (struct Win *) = NULL;
    if (   match_func(c, &f, 'c', draw_win_inventory)
        || match_func(c, &f, 'i', draw_win_info)
        || match_func(c, &f, 'l', draw_win_log)
        || match_func(c, &f, 'k', draw_win_available_keybindings)
        || match_func(c, &f, 'm', draw_win_map)
        || match_func(c, &f, '0', draw_win_keybindings_global)
        || match_func(c, &f, '1', draw_win_keybindings_winconf_geometry)
        || match_func(c, &f, '2', draw_win_keybindings_winconf_keybindings))
    {
        ;
    }
    return f;
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
        draw_wins(next);
    }
}



extern uint8_t get_win_pos_in_order(char c)
{
    uint8_t i;
    for (i = 0; c != world.winDB.order[i]; i++);
    return i;
}



extern struct Win * get_win_after(char c)
{
    return get_win_by_id(world.winDB.order[get_win_pos_in_order(c) + 1]);
}



extern uint16_t center_offset(uint16_t position, uint32_t mapsize,
                              uint32_t frame_size)
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
    char * context = "Failed reading individual window's configuration from "
                     "interface config file. ";
    char * err_id  = "Illegal ID(s) selected.";
    char * err_2   = "Double-initialized window.";
    char * err_brk = "Illegal line break type index.";
    int test_for_end = try_fgetc(file, f_name);
    if (EOF == test_for_end || '\n' == test_for_end)
    {
        return 0;
    }
    exit_trouble(EOF == ungetc(test_for_end, file), f_name, "ungetc()");
    err_try_fgets(line, linemax, file, context, "ns");
    err_line(NULL == strchr(world.winDB.legal_ids, line[0]), line, context,
             err_id);
    exit_err(world.winDB.ids && NULL!=strchr(world.winDB.ids, line[0]), err_2);
    struct Win win;
    memset(&win, 0, sizeof(struct Win));
    win.id = (char) line[0];
    err_try_fgets(line, linemax, file, context, "0n");
    win.title = try_malloc(strlen(line), f_name);
    memcpy(win.title, line, strlen(line) - 1);      /* Eliminate newline char */
    win.title[strlen(line) - 1] = '\0';             /* char at end of string. */
    err_try_fgets(line, linemax, file, context, "0nsi");
    err_line(atoi(line) > 2, line, context, err_brk);
    win.linebreak = atoi(line);
    err_try_fgets(line, linemax, file, context, "0ni");
    win.target_height = atoi(line);
    win.target_height_type = (0 >= win.target_height);
    err_try_fgets(line, linemax, file, context, "0ni");
    win.target_width = atoi(line);
    win.target_width_type = (0 >= win.target_width);
    read_keybindings_from_file(line, linemax, file, &win.kb);
    if (world.winDB.ids)
    {
        uint8_t old_ids_size = strlen(world.winDB.ids);
        char * new_ids = try_malloc(old_ids_size + 1 + 1, f_name);
        sprintf(new_ids, "%s%c", world.winDB.ids, win.id);
        free(world.winDB.ids);
        world.winDB.ids = new_ids;
        array_append(old_ids_size, sizeof(struct Win), (void *) &win,
                     (void **) &world.winDB.wins);
        return 1;
    }
    world.winDB.ids = try_malloc(2, f_name);
    sprintf(world.winDB.ids, "%c", win.id);
    world.winDB.wins = try_malloc(sizeof(struct Win), f_name);
    world.winDB.wins[0] = win;
    return 1;
}



extern void write_winconf_of_id_to_file(FILE * file, char c)
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
    sprintf(line, "%d\n", wc->linebreak);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%d\n", wc->target_height);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%d\n", wc->target_width);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    write_keybindings_to_file(file, &wc->kb);
}



extern void read_order_wins_visible_active(char * line, uint32_t linemax,
                                           FILE * file, char ** tmp_order,
                                           char * tmp_active)
{
    char * f_name = "read_order_wins_visible_active()";
    char * context   = "Failed reading order and activation of visible windows "
                       "from interface config file. ";
    char * err_id    = "Illegal ID(s) selected.";
    err_try_fgets(line, linemax, file, context, "01");
    uint32_t i = 0;
    for (; i < strlen(line) - 1; i++)
    {
        char * test = strchr(world.winDB.legal_ids, line[i]);
        err_line(!test, line, context, err_id);
    }
    line[strlen(line) - 1] = '\0';
    *tmp_order = try_malloc(strlen(line) + 1, f_name);
    sprintf(*tmp_order, "%s", line);
    if (*tmp_order[0])
    {
        err_try_fgets(line, linemax, file, context, "0nfs");
        err_line(NULL == strchr(*tmp_order, line[0]), line, context, err_id);
        *tmp_active = line[0];
    }
    else
    {
        err_try_fgets(line, linemax, file, context, "0ne");
        *tmp_active = '\0';
    }
    err_try_fgets(line, linemax, file, context, "d");
}



extern void write_order_wins_visible_active(FILE * file)
{
    char * f_name = "write_order_wins_visible_active()";
    try_fwrite(world.winDB.order, strlen(world.winDB.order), 1, file, f_name);
    try_fputc('\n', file, f_name);
    try_fputc(world.winDB.active, file, f_name);
    try_fputc('\n', file, f_name);
    try_fwrite(world.delim, strlen(world.delim), 1, file, f_name);
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
        free(wc->kb.kbs);
        wc->kb.kbs = NULL;
    }
    free(world.winDB.ids); /* NULL this since read_winconf_from_file() checks */
    world.winDB.ids = NULL;/* for it to detect its first post-DB-purge round. */
    free(world.winDB.wins);
    free(world.winDB.order);
}



extern void winch_called()
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
