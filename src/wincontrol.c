/* wincontrol.c */

#include "wincontrol.h"
#include <stdlib.h> /* for free() */
#include <string.h> /* for strlen(), strchr(), strstr() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include "windows.h" /* for suspend_win(), append_win(), reset_pad_offset(),
                      * resize_active_win(), init_win(), free_win(), struct Win
                      */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "main.h" /* for world global */
#include "readwrite.h" /* for get_linemax(), try_fopen(), try_fclose(),
                        * try_fgets(), try_fclose_unlink_rename(), try_fwrite()
                        */
#include "rexit.h" /* for exit_err() */
#include "draw_wins.h" /* for draw_win_map(), draw_win_info(), draw_win_log(),
                        * draw_win_available_keybindings(),
                        * draw_win_inventory(), draw_win_keybindings_global(),
                        * draw_win_keybindings_winconf_geometry(),
                        * draw_win_keybindings_winconf_keybindings(),
                        * draw_winconf_geometry(), draw_winconf_keybindings()
                        */
#include "misc.h" /* for try_malloc(), exit_trouble() */
#include "dirent.h" /* for opendir(), closedir(), readdir() */
#include "errno.h" /* for errno */
#include "keybindings.h" /* for KeyBinding struct, free_keybindings() */



/* Return string "prefix" + "id"; malloc()'s string, remember to call free()! */
static char * string_prefixed_id(char * prefix, char id);

/* Initializes Winconf: .view/.height_type/.width_type to 0, .id to "id". */
static void create_winconf(char id, struct WinConf * wcp);

/* Initialize Winconf of "id" from appropriate config file.*/
static void init_winconf_from_file(char id);

/* Wrapper around init_win() called with values from Winconf of "id". */
static void init_win_from_winconf(char id);

/* Save title, draw function, size of window identified by "id" to conffile. */
static void save_win_config(char id);

/* Free data pointed to inside individual WinConf struct of "id". */
static void free_winconf_data(char id);

/* Write geometry of a window to its WinConf, as positive or negative values
 * (dependent on state ofWinConf->height_type / WinConf->width_type).
 */
static void set_winconf_geometry(char id);

/* Get WinConf by "id"; get id of WinConf mothering "win". */
static struct WinConf * get_winconf_by_id(char id);

/* Get (Win->draw) function identified by "c"; NULL if c not mapped to one. */
static void * get_drawfunc_by_char(char c);

/* Iterate over chars of world.winconf_ids array. Re-start after null byte. */
static char get_next_winconf_id();



static char * string_prefixed_id(char * prefix, char id)
{
    uint8_t size = strlen(prefix) + 2;
    char * path = try_malloc(size, "string_prefixed_id()");
    sprintf(path, "%s_", prefix);
    path[size - 2] = id;
    return path;
}



static void create_winconf(char id, struct WinConf * wcp)
{
    wcp->id = id;
    wcp->view = 0;
    wcp->height_type = 0;
    wcp->width_type = 0;
    wcp->kb.edit = 0;
    wcp->kb.select = 0;
}



static void init_winconf_from_file(char id)
{
    char * tmp = "init_winconf_from_file() on window id '_'";
    char * context = try_malloc(strlen(tmp) + 1, "init_winconf_from_file()");
    memcpy(context, tmp, strlen(tmp) + 1);
    context[strlen(tmp) - 2] = id;

    char * path = string_prefixed_id("config/windows/Win_", id);
    FILE * file = try_fopen(path, "r", context);
    free(path);
    uint16_t linemax = get_linemax(file, context);
    char line[linemax + 1];

    struct WinConf * winconf = get_winconf_by_id(id);
    try_fgets(line, linemax + 1, file, context);
    winconf->title = try_malloc(strlen(line), context);
    memcpy(winconf->title, line, strlen(line) - 1); /* Eliminate newline char */
    winconf->title[strlen(line) - 1] = '\0';        /* char at end of string. */

    try_fgets(line, linemax + 1, file, context);
    winconf->draw = line[0];

    try_fgets(line, linemax + 1, file, context);
    winconf->height = atoi(line);
    if (0 >= winconf->height)
    {
        winconf->height_type = 1;
    }
    try_fgets(line, linemax + 1, file, context);
    winconf->width = atoi(line);
    if (0 >= winconf->width)
    {
        winconf->width_type = 1;
    }

    char command[linemax + 1];
    char * cmdptr;
    struct KeyBinding ** loc_last_ptr = &winconf->kb.kbs;
    * loc_last_ptr = 0;
    while (fgets(command, linemax + 1, file))
    {
        if ('\n' == command[0] || 0 == command[0])
        {
            break;
        }
        * loc_last_ptr = try_malloc(sizeof(struct KeyBinding), context);
        struct KeyBinding * kb_p = * loc_last_ptr;
        kb_p->next = 0;
        kb_p->key = atoi(command);
        cmdptr = strchr(command, ' ') + 1;
        kb_p->name = try_malloc(strlen(cmdptr), context);
        memcpy(kb_p->name, cmdptr, strlen(cmdptr) - 1);
        kb_p->name[strlen(cmdptr) - 1] = '\0';
        loc_last_ptr = & kb_p->next;
    }

    try_fclose(file, context);
    free(context);
}



static void init_win_from_winconf(char id)
{
    char * err = "get_drawfunc_by_char() returns NULL to init_win_from_file().";
    struct WinConf * winconf = get_winconf_by_id(id);
    void * f = get_drawfunc_by_char(winconf->draw);
    exit_err(NULL == f, err);
    init_win(&winconf->win, winconf->title, winconf->height, winconf->width, f);
}



static void save_win_config(char id)
{
    char * f_name = "save_win_config()";

    char * path_tmp = string_prefixed_id("config/windows/Win_tmp_", id);
    FILE * file = try_fopen(path_tmp, "w", f_name);

    struct WinConf * wc = get_winconf_by_id(id);
    uint8_t size = strlen(wc->title) + 2;
    if (size < 7)
    {
        size = 7;
    }
    char line[size];
    sprintf(line, "%s\n", wc->title);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%c\n", wc->draw);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%d\n", wc->height);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    sprintf(line, "%d\n", wc->width);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);

    uint16_t linemax = 0;
    struct KeyBinding * kb_p = wc->kb.kbs;
    while (0 != kb_p)
    {
        if (strlen(kb_p->name) > linemax)
        {
            linemax = strlen(kb_p->name);
        }
        kb_p = kb_p->next;
    }
    linemax = linemax + 6;         /* + 6 = + 3 digits + whitespace + \n + \0 */

    char kb_line[linemax];
    kb_p = wc->kb.kbs;
    while (0 != kb_p)
    {
        snprintf(kb_line, linemax, "%d %s\n", kb_p->key, kb_p->name);
        try_fwrite(kb_line, sizeof(char), strlen(kb_line), file, f_name);
        kb_p = kb_p->next;
    }

    char * path = string_prefixed_id("config/windows/Win_", id);
    try_fclose_unlink_rename(file, path_tmp, path, f_name);
    free(path);
    free(path_tmp);
}



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
        wcp->height = wcp->win->framesize.y - world.wmeta->padsize.y + 1;
    }
    if      (0 == wcp->width_type)
    {
        wcp->width = wcp->win->framesize.x;
    }
    else if (1 == wcp->width_type)
    {
        wcp->width = wcp->win->framesize.x - world.wmeta->padsize.x;
    }
}



static struct WinConf * get_winconf_by_id(char id)
{
    uint8_t i = 0;
    while (1)
    {
        if (id == world.winconfs[i].id)
        {
            return &world.winconfs[i];
        }
        i++;
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



static char get_next_winconf_id()
{
    static uint8_t i = 0;
    char c = world.winconf_ids[i];
    if (0 == c)
    {
        i = 0;
        return c;
    }
    i++;
    return c;
}



extern struct WinConf * get_winconf_by_win(struct Win * win)
{
    uint8_t i = 0;
    while (1)
    {
        if (win == world.winconfs[i].win)
        {
            return &world.winconfs[i];
        }
        i++;
    }
}



extern struct Win * get_win_by_id(char id)
{
    struct WinConf * wc = get_winconf_by_id(id);
    return wc->win;
}



extern void init_winconfs()
{
    char * f_name = "init_winconfs()";

    DIR * dp = opendir("config/windows");
    exit_trouble(NULL == dp, f_name, "opendir()");
    struct dirent * fn;
    errno = 0;
    char * winconf_ids = try_malloc(256, f_name);
    uint8_t i = 0;
    char id;
    while (NULL != (fn = readdir(dp)))
    {
        if (5 == strlen(fn->d_name) && fn->d_name == strstr(fn->d_name, "Win_"))
        {
            id = fn->d_name[4];
            winconf_ids[i] = id;
            i++;
        }
    }
    winconf_ids[i] = '\0';
    exit_trouble(errno, f_name, "readdir()");
    exit_trouble(closedir(dp), f_name, "closedir()");
    world.winconf_ids = try_malloc(strlen(winconf_ids) + 1, f_name);
    memcpy(world.winconf_ids, winconf_ids, strlen(winconf_ids) + 1);
    free(winconf_ids);

    struct WinConf * winconfs;
    winconfs = try_malloc(strlen(world.winconf_ids) * sizeof(struct WinConf),
                          f_name);
    i = 0;
    while (0 != (id = get_next_winconf_id()))
    {
        create_winconf(id, &winconfs[i]);
        i++;
    }
    world.winconfs = winconfs;
    while (0 != (id = get_next_winconf_id()))
    {
        init_winconf_from_file(id);
        i++;
    }
}



extern void free_winconfs()
{
    char id;
    while (0 != (id = get_next_winconf_id()))
    {
        free_winconf_data(id);
    }
    free(world.winconf_ids);
    free(world.winconfs);
}



extern void init_wins()
{
    char id;
    while (0 != (id = get_next_winconf_id()))
    {
        init_win_from_winconf(id);
    }
}



extern void sorted_wintoggle_and_activate()
{
    char * f_name = "sorted_wintoggle_and_activate()";

    char * path = "config/windows/toggle_order_and_active";
    FILE * file = try_fopen(path, "r", f_name);
    uint16_t linemax = get_linemax(file, f_name);

    char win_order[linemax + 1];
    try_fgets(win_order, linemax + 1, file, f_name);

    uint8_t a = 0;
    exit_trouble(read_uint8(file, &a), f_name, "read_uint8()");

    try_fclose(file, f_name);

    uint8_t i = 0;
    for (; i < linemax - 1; i++)
    {
        if (NULL == strchr(world.winconf_ids, win_order[i]))
        {
            continue;
        }
        toggle_window(win_order[i]);

        if (a == (uint8_t) win_order[i])
        {
            world.wmeta->active = get_win_by_id(win_order[i]);
        }
    }
}



extern void save_win_configs()
{
    char * f_name = "save_win_configs()";

    char id;
    while (0 != (id = get_next_winconf_id()))
    {
        save_win_config(id);
    }

    char * path     = "config/windows/toggle_order_and_active";
    char * path_tmp = "config/windows/toggle_order_and_active_tmp";
    FILE * file = try_fopen(path_tmp, "w", f_name);

    char line[6];
    struct Win * w_p = world.wmeta->chain_start;
    uint8_t i = 0;
    while (0 != w_p)
    {
        struct WinConf * wc = get_winconf_by_win(w_p);
        line[i] = wc->id;
        w_p = w_p->next;
        i++;
    }
    line[i] = '\n';
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    if (0 != world.wmeta->active)
    {
        struct WinConf * wc = get_winconf_by_win(world.wmeta->active);
        write_uint8(wc->id, file);
    }

    try_fclose_unlink_rename(file, path_tmp, path, f_name);
}



extern void toggle_window(char id)
{
    struct Win * win = get_win_by_id(id);
    if (0 == win->prev && world.wmeta->chain_start != win)  /* Win struct is  */
    {                                                       /* outside chain. */
        append_win(win);
    }
    else
    {
        suspend_win(win);
    }
}



extern void toggle_winconfig()
{
   struct Win * win = world.wmeta->active;
   struct WinConf * wcp = get_winconf_by_win(win);
    if      (0 == wcp->view)
    {
        win->draw = draw_winconf_geometry;
        wcp->view = 1;
        wcp->center = win->center;
        win->center.y = 0;
        win->center.x = 0;
    }
    else if (1 == wcp->view)
    {
        win->draw = draw_winconf_keybindings;
        wcp->view = 2;
        win->center.x = 0;
    }
    else
    {
        win->draw = get_drawfunc_by_char(wcp->draw);
        win->center = wcp->center;
        wcp->view = 0;
    }
}



extern void toggle_win_height_type()
{
    struct Win * win = world.wmeta->active;
    struct WinConf * wcp = get_winconf_by_win(win);
    if (0 == wcp->height_type)
    {
        wcp->height_type = 1;
    }
    else
    {
        wcp->height_type = 0;
    }
    set_winconf_geometry(wcp->id);
}



extern void toggle_win_width_type()
{
    struct Win * win = world.wmeta->active;
    struct WinConf * wcp = get_winconf_by_win(win);
    if (0 == wcp->width_type && win->framesize.x <= world.wmeta->padsize.x)
    {
        wcp->width_type = 1;
    }
    else
    {
        wcp->width_type = 0;
    }
    set_winconf_geometry(wcp->id);
}



extern void scroll_pad(char dir)
{
    if      ('+' == dir)
    {
        reset_pad_offset(world.wmeta->pad_offset + 1);
    }
    else if ('-' == dir)
    {
        reset_pad_offset(world.wmeta->pad_offset - 1);
    }
}



extern void growshrink_active_window(char change)
{
    if (0 != world.wmeta->active)
    {
        struct yx_uint16 size = world.wmeta->active->framesize;
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
        struct WinConf * wcp = get_winconf_by_win(world.wmeta->active);
        if (   1 == wcp->width_type
            && world.wmeta->active->framesize.x > world.wmeta->padsize.x)
        {
            wcp->width_type = 0;
        }
        set_winconf_geometry(wcp->id);
    }
}
