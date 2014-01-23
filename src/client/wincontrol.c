/* src/client/wincontrol.c */

#include "wincontrol.h"
#include <errno.h> /* global errno */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* strlen(), strchr(), memcpy() */
#include "../common/readwrite.h" /* try_fgets(), try_fwrite(), try_fgetc(),
                                  * try_fputc()
                                  */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint16.h" /* struct yx_uint16 */
#include "draw_wins.h" /* draw_win_map(), draw_win_info(), draw_win_log(),
                        * draw_win_available_keybindings(),
                        * draw_win_inventory(), draw_win_keybindings_global(),
                        * draw_win_keybindings_winconf_geometry(),
                        * draw_win_keybindings_winconf_keybindings(),
                        * draw_winconf_geometry(), draw_winconf_keybindings()
                        */
#include "keybindings.h" /* struct KeyBinding, free_keybindings() */
#include "windows.h" /* struct Win, resize_active_win(), reset_pad_offset(),
                      * append_win(), suspend_win(), init_win(), free_win()
                      */
#include "world.h" /* global world */



/* Wrapper around init_win() called with values from Winconf of "id". */
static void init_win_from_winconf(char id);

/* Free data pointed to inside individual WinConf struct of "id". */
static void free_winconf_data(char id);

/* Write geometry of a window to its WinConf, as positive or negative values
 * (dependent on state ofWinConf->height_type / WinConf->width_type).
 */
static void set_winconf_geometry(char id);

/* Get WinConf by "id". */
static struct WinConf * get_winconf_by_id(char id);

/* Get (Win->draw) function identified by "c"; NULL if c not mapped to one. */
static void * get_drawfunc_by_char(char c);



static void free_winconf_data(char id)
{
    struct WinConf * wc = get_winconf_by_id(id);
    free(wc->title);
    free_keybindings(wc->kb.kbs);
    free_win(wc->win);
}



static void set_winconf_geometry(char id)
{
    struct WinConf * wcp = get_winconf_by_id(id);
    if      (0 == wcp->height_type)
    {
        wcp->height = wcp->win->framesize.y;
    }
    else if (1 == wcp->height_type)
    {
        wcp->height = wcp->win->framesize.y - world.wins.padsize.y + 1;
    }
    if      (0 == wcp->width_type)
    {
        wcp->width = wcp->win->framesize.x;
    }
    else if (1 == wcp->width_type)
    {
        wcp->width = wcp->win->framesize.x - world.wins.padsize.x;
    }
}



static void * get_drawfunc_by_char(char c)
{

    if      ('c' == c)
    {
        return draw_win_inventory;
    }
    else if ('i' == c)
    {
        return draw_win_info;
    }
    else if ('l' == c)
    {
        return draw_win_log;
    }
    else if ('k' == c)
    {
        return draw_win_available_keybindings;
    }
    else if ('m' == c)
    {
        return draw_win_map;
    }
    else if ('0' == c)
    {
        return draw_win_keybindings_global;
    }
    else if ('1' == c)
    {
        return draw_win_keybindings_winconf_geometry;
    }
    else if ('2' == c)
    {
        return draw_win_keybindings_winconf_keybindings;
    }
    return NULL;
}



extern void init_win_from_winconf(char id)
{
    char * err = "get_drawfunc_by_char() returns NULL to init_win_from_file().";
    struct WinConf * winconf = get_winconf_by_id(id);
    void * f = get_drawfunc_by_char(winconf->id);
    exit_err(NULL == f, err);
    init_win(&winconf->win, winconf->title, winconf->height, winconf->width, f);
}



static struct WinConf * get_winconf_by_id(char id)
{
    uint8_t i = 0;
    while (1)
    {
        if (id == world.wins.winconfs[i].id)
        {
            return &world.wins.winconfs[i];
        }
        i++;
    }
}



extern struct WinConf * get_winconf_by_win(struct Win * win)
{
    uint8_t i = 0;
    while (1)
    {
        if (win == world.wins.winconfs[i].win)
        {
            return &world.wins.winconfs[i];
        }
        i++;
    }
}



extern struct Win * get_win_by_id(char id)
{
    struct WinConf * wc = get_winconf_by_id(id);
    return wc->win;
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
    struct WinConf winconf;
    winconf.id = (char) test;
    try_fgetc(file, f_name);
    try_fgets(line, linemax + 1, file, f_name);
    winconf.title = try_malloc(strlen(line), f_name);
    memcpy(winconf.title, line, strlen(line) - 1);  /* Eliminate newline char */
    winconf.title[strlen(line) - 1] = '\0';         /* char at end of string. */
    try_fgets(line, linemax + 1, file, f_name);
    winconf.height = atoi(line);
    winconf.height_type = (0 >= winconf.height);
    try_fgets(line, linemax + 1, file, f_name);
    winconf.width = atoi(line);
    winconf.width_type = (0 >= winconf.width);
    read_keybindings_from_file(line, linemax, file, &winconf.kb);
    winconf.win = NULL;
    winconf.view = 0;
    winconf.center.y = 0;
    winconf.center.x = 0;
    if (world.wins.ids)
    {
        uint8_t old_ids_size = strlen(world.wins.ids);
        char * new_ids = try_malloc(old_ids_size + 1 + 1, f_name);
        sprintf(new_ids, "%s%c", world.wins.ids, winconf.id);
        free(world.wins.ids);
        world.wins.ids = new_ids;
        uint16_t old_winconfs_size = old_ids_size * sizeof(struct WinConf);
        uint16_t new_winconfs_size = old_winconfs_size + sizeof(struct WinConf);
        struct WinConf * new_winconfs = try_malloc(new_winconfs_size, f_name);
        memcpy(new_winconfs, world.wins.winconfs, old_winconfs_size);
        new_winconfs[old_ids_size] = winconf;
        free(world.wins.winconfs);
        world.wins.winconfs = new_winconfs;
    }
    else
    {
        world.wins.ids = try_malloc(2, f_name);
        sprintf(world.wins.ids, "%c", winconf.id);
        world.wins.winconfs = try_malloc(sizeof(struct WinConf), f_name);
        *world.wins.winconfs = winconf;
    }
    return 1;
}



extern void write_winconf_of_id_to_file(FILE * file, char c, char * delim)
{
    char * f_name = "write_winconf_of_id_to_file()";
    struct WinConf * wc = get_winconf_by_id(c);
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
    sprintf(line, "%d\n", wc->height);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%d\n", wc->width);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    write_keybindings_to_file(file, &wc->kb, delim);
}



extern void read_order_wins_visible_active(char * line, uint32_t linemax,
                                           FILE * file)
{
    char * f_name = "read_order_wins_visible_active()";
    char win_order[linemax + 1];
    try_fgets(win_order, linemax + 1, file, f_name);
    world.wins.order = try_malloc(linemax, f_name);
    win_order[strlen(win_order) - 1] = '\0';
    sprintf(world.wins.order, "%s", win_order);
    int char_or_eof = try_fgetc(file, f_name);
    char * err_eof = "fgetc() unexpectedly hitting EOF";
    exit_trouble(EOF == char_or_eof, f_name, err_eof);
    world.wins.id_active = (uint8_t) char_or_eof;
    exit_trouble(EOF == try_fgetc(file, f_name), f_name, err_eof);
    try_fgets(line, linemax + 1, file, f_name);
}



extern void write_order_wins_visible_active(FILE * file, char * delim)
{
    char * f_name = "write_order_wins_visible_active()";
    char line[strlen(world.wins.ids) + 2];
    struct Win * w_p = world.wins.chain_start;
    char active = ' ';
    uint8_t i;
    for (; NULL != w_p; w_p = w_p->next, i++)
    {
        struct WinConf * wc_p = get_winconf_by_win(w_p);
        line[i] = wc_p->id;
        if (w_p == world.wins.win_active)
        {
            active = wc_p->id;
        }
    }
    line[i] = '\n';
    line[i + 1] = '\0';
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    try_fputc(active, file, f_name);
    try_fputc('\n', file, f_name);
    try_fwrite(delim, strlen(delim), 1, file, f_name);
}



extern void free_winconfs()
{
    char id;
    while (0 != (id = get_next_winconf_id()))
    {
        free_winconf_data(id);
    }
    free(world.wins.ids);
    world.wins.ids = NULL;
    free(world.wins.winconfs);
    world.wins.winconfs = NULL;
    free(world.wins.order);
    world.wins.order = NULL;
}



extern void init_wins()
{
    char id;
    while (0 != (id = get_next_winconf_id()))
    {
        init_win_from_winconf(id);
    }
}



extern void sorted_win_toggle_and_activate()
{
    uint8_t i = 0;
    for (; i < strlen(world.wins.order); i++)
    {
        if (NULL == strchr(world.wins.ids, world.wins.order[i]))
        {
            continue;
        }
        toggle_window(world.wins.order[i]);
        if (world.wins.id_active == (uint8_t) world.wins.order[i])
        {
            world.wins.win_active = get_win_by_id(world.wins.order[i]);
        }
    }
}


extern char get_next_winconf_id()
{
    static uint8_t i = 0;
    char c = world.wins.ids[i];
    if (0 == c)
    {
        i = 0;
        return c;
    }
    i++;
    return c;
}



extern void toggle_window(char id)
{
    struct Win * win = get_win_by_id(id);
    if (0 == win->prev && world.wins.chain_start != win)    /* Win struct is  */
    {                                                       /* outside chain? */
        append_win(win);
    }
    else
    {
        suspend_win(win);
    }
}



extern void toggle_winconfig()
{
   struct Win * win = world.wins.win_active;
   struct WinConf * wcp = get_winconf_by_win(win);
    if      (0 == wcp->view)
    {
        wcp->view     = 1;
        win->draw     = draw_winconf_geometry;
        wcp->center   = win->center;
        win->center.y = 0;
        win->center.x = 0;
    }
    else if (1 == wcp->view)
    {
        wcp->view     = 2;
        win->draw     = draw_winconf_keybindings;
        win->center.x = 0;
    }
    else
    {
        wcp->view     = 0;
        win->draw     = get_drawfunc_by_char(wcp->id);
        win->center   = wcp->center;
    }
}



extern void toggle_win_size_type(char axis)
{
    struct Win * win = world.wins.win_active;
    struct WinConf * wcp = get_winconf_by_win(win);
    if ('y' == axis)
    {
        wcp->height_type = (0 == wcp->height_type);
        set_winconf_geometry(wcp->id);
        return;
    }
    wcp->width_type = (   0 == wcp->width_type
                       && win->framesize.x <= world.wins.padsize.x);
    set_winconf_geometry(wcp->id);
}



extern void scroll_pad(char dir)
{
    if      ('+' == dir)
    {
        reset_pad_offset(world.wins.pad_offset + 1);
    }
    else if ('-' == dir)
    {
        reset_pad_offset(world.wins.pad_offset - 1);
    }
}



extern void growshrink_active_window(char change)
{
    if (0 != world.wins.win_active)
    {
        struct yx_uint16 size = world.wins.win_active->framesize;
        if      (change == '-')
        {
            size.y--;
        }
        else if (change == '+')
        {
            size.y++;
        }
        else if (change == '_')
        {
            size.x--;
        }
        else if (change == '*')
        {
            size.x++;
        }
        resize_active_win(size);
        struct WinConf * wcp = get_winconf_by_win(world.wins.win_active);
        if (   1 == wcp->width_type
            && world.wins.win_active->framesize.x > world.wins.padsize.x)
        {
            wcp->width_type = 0;
        }
        set_winconf_geometry(wcp->id);
    }
}
