/* wincontrol.c */

#include "wincontrol.h"
#include <stdlib.h> /* for free() */
#include <string.h> /* for strlen(), strchr(), strstr() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include <unistd.h> /* for access(), unlink() */
#include "windows.h" /* for suspend_win(), append_win(), reset_pad_offset(),
                      * resize_active_win(), init_win(), free_win(),
                      * structs Win, WinMeta
                      */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "main.h" /* for Wins struct */
#include "readwrite.h" /* for get_linemax(), try_fopen(), try_fclose(),
                        * try_fgets(), try_fclose_unlink_rename(), try_fwrite()
                        */
#include "rexit.h" /* for exit_err() */
#include "main.h" /* for World struct */
#include "draw_wins.h" /* for draw_win_map(), draw_win_info(), draw_win_og(),
                        * draw_win_available_keybindings(),
                        * draw_win_keybindings_global(), draw_win_inventory(),
                        * draw_win_keybindings_winconf_geometry(),
                        * draw_win_keybindings_winconf_keybindings(),
                        * draw_winconf_geometry(), draw_winconf_keybindings()
                        */
#include "misc.h" /* for try_malloc(), trouble_msg() */
#include "dirent.h" /* for opendir(), closedir(), readdir() */
#include "errno.h" /* for errno */
#include "keybindings.h" /* for KeyBinding struct, free_keybindings() */



/* Return string "prefix" + "id"; malloc()'s string, remember to call free()! */
static char * string_prefixed_id(struct World * world, char * prefix, char id);



/* Create Winconf, init ->view/height_type/width_type to 0, ->id to "id". */
static void create_winconf(struct World * world, char id, struct WinConf * wcp);

/* Initialize Winconf of "id" from appropriate config file.*/
static void init_winconf_from_file(struct World * world, char id);

/* Wrapper around init_win() called with values from Winconf of "id". */
static void init_win_from_winconf(struct World * world, char id);

/* Save title, draw function, size of window identified by "id" to conffile. */
static void save_win_config(struct World * world, char id);

/* Free data pointed to inside WinConf struct. */
static void free_winconf_data(struct World * world, char id);



/* Write geometry of a window to its WinConf, as positive or negative values
 * (dependent on state ofWinConf->height_type / WinConf->width_type).
 */
static void set_winconf_geometry(struct World * world, char id);



/* Get WinConf by "id"; get id of WinConf mothering "win". */
static struct WinConf * get_winconf_by_id(struct World * world, char id);

/* Get (Win->draw) function identified by "c"; NULL if c not mapped to one. */
static void * get_drawfunc_by_char(char c);

/* Iterate over bytes of world->winconf_ids array. Re-start after null byte. */
static char get_next_winconf_id(struct World * world);



static char * string_prefixed_id(struct World * world, char * prefix, char id)
{
    uint8_t size = strlen(prefix) + 2;
    char * path = try_malloc(size, world, "string_prefixed_id()");
    sprintf(path, "%s_", prefix);
    path[size - 2] = id;
    return path;
}



static void create_winconf(struct World * world, char id, struct WinConf * wcp)
{
    wcp->id = id;
    wcp->view = 0;
    wcp->height_type = 0;
    wcp->width_type = 0;
    wcp->kb.edit = 0;
    wcp->kb.select = 0;
}



static void init_winconf_from_file(struct World * world, char id)
{
    char * tmp = "init_winconf_from_file() on window id '_'";
    char * context = try_malloc(strlen(tmp) + 1, world,
                               "init_winconf_from_file()");
    memcpy(context, tmp, strlen(tmp) + 1);
    context[strlen(tmp) - 2] = id;

    char * path = string_prefixed_id(world, "config/windows/Win_", id);
    FILE * file = try_fopen(path, "r", world, context);
    free(path);
    uint16_t linemax = get_linemax(file, world, context);
    char line[linemax + 1];

    struct WinConf * winconf = get_winconf_by_id(world, id);
    try_fgets(line, linemax + 1, file, world, context);
    winconf->title = try_malloc(strlen(line), world, context);
    memcpy(winconf->title, line, strlen(line) - 1); /* Eliminate newline char */
    winconf->title[strlen(line) - 1] = '\0';        /* char at end of string. */

    try_fgets(line, linemax + 1, file, world, context);
    winconf->draw = line[0];

    try_fgets(line, linemax + 1, file, world, context);
    winconf->height = atoi(line);
    if (0 >= winconf->height)
    {
        winconf->height_type = 1;
    }
    try_fgets(line, linemax + 1, file, world, context);
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
        * loc_last_ptr = try_malloc(sizeof(struct KeyBinding), world, context);
        struct KeyBinding * kb_p = * loc_last_ptr;
        kb_p->next = 0;
        kb_p->key = atoi(command);
        cmdptr = strchr(command, ' ') + 1;
        kb_p->name = try_malloc(strlen(cmdptr), world, context);
        memcpy(kb_p->name, cmdptr, strlen(cmdptr) - 1);
        kb_p->name[strlen(cmdptr) - 1] = '\0';
        loc_last_ptr = & kb_p->next;
    }

    try_fclose(file, world, context);
    free(context);
}



static void init_win_from_winconf(struct World * world, char id)
{
    char * tmp = "Trouble in init_win_from_file() with init_win() (win id: _).";
    char * err = try_malloc(strlen(tmp) + 1, world, "init_win_from_file()");
    memcpy(err, tmp, strlen(tmp) + 1);
    err[strlen(tmp) - 3] = id;
    struct WinConf * winconf = get_winconf_by_id(world, id);
    void * f = get_drawfunc_by_char(winconf->draw);
    exit_err(NULL == f, world, err);
    exit_err(init_win(world->wmeta, &winconf->win, winconf->title,
                      winconf->height, winconf->width, world, f),
             world, err);
    free(err);
}



static void save_win_config(struct World * world, char id)
{
    char * f_name = "save_win_config()";

    char * path_tmp = string_prefixed_id(world, "config/windows/Win_tmp_", id);
    FILE * file = try_fopen(path_tmp, "w", world, f_name);

    struct WinConf * wc = get_winconf_by_id(world, id);
    uint8_t size = strlen(wc->title) + 2;
    if (size < 7)
    {
        size = 7;
    }
    char line[size];
    sprintf(line, "%s\n", wc->title);
    try_fwrite(line, sizeof(char), strlen(line), file, world, f_name);
    sprintf(line, "%c\n", wc->draw);
    try_fwrite(line, sizeof(char), strlen(line), file, world, f_name);
    sprintf(line, "%d\n", wc->height);
    try_fwrite(line, sizeof(char), strlen(line), file, world, f_name);
    sprintf(line, "%d\n", wc->width);
    try_fwrite(line, sizeof(char), strlen(line), file, world, f_name);

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

    char keyb_line[linemax];
    kb_p = wc->kb.kbs;
    while (0 != kb_p)
    {
        snprintf(keyb_line, linemax, "%d %s\n", kb_p->key, kb_p->name);
        try_fwrite(keyb_line, sizeof(char), strlen(keyb_line), file, world, f_name);
        kb_p = kb_p->next;
    }

    char * path = string_prefixed_id(world, "config/windows/Win_", id);
    try_fclose_unlink_rename(file, path_tmp, path, world, f_name);
    free(path);
    free(path_tmp);
}



static void free_winconf_data(struct World * world, char id)
{
    struct WinConf * wc = get_winconf_by_id(world, id);
    free(wc->title);
    free_keybindings(wc->kb.kbs);
    free_win(wc->win);
}



static void set_winconf_geometry(struct World * world, char id)
{
    struct WinConf * wcp = get_winconf_by_id(world, id);
    if      (0 == wcp->height_type)
    {
        wcp->height = wcp->win->frame.size.y;
    }
    else if (1 == wcp->height_type)
    {
        wcp->height = wcp->win->frame.size.y - world->wmeta->padframe.size.y
                      + 1;
    }
    if      (0 == wcp->width_type)
    {
        wcp->width = wcp->win->frame.size.x;
    }
    else if (1 == wcp->width_type)
    {
        wcp->width = wcp->win->frame.size.x - world->wmeta->padframe.size.x;
    }
}



static struct WinConf * get_winconf_by_id(struct World * world, char id)
{
    uint8_t i = 0;
    while (1)
    {
        if (id == world->winconfs[i].id)
        {
            return &world->winconfs[i];
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
    if      ('i' == c)
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



static char get_next_winconf_id(struct World * world)
{
    static uint8_t i = 0;
    char c = world->winconf_ids[i];
    if (0 == c)
    {
        i = 0;
    }
    else
    {
        i++;
    }
    return c;
}



extern struct WinConf * get_winconf_by_win(struct World * world,
                                           struct Win * win)
{
    uint8_t i = 0;
    while (1)
    {
        if (win == world->winconfs[i].win)
        {
            return &world->winconfs[i];
        }
        i++;
    }
}



extern struct Win * get_win_by_id(struct World * world, char id)
{
    struct WinConf * wc = get_winconf_by_id(world, id);
    return wc->win;
}



extern void init_winconfs(struct World * world)
{
    char * f_name = "init_winconfs()";
    char * err_o = "Trouble in init_winconfs() with opendir().";
    char * err_r = "Trouble in init_winconfs() with readdir().";
    char * err_c = "Trouble in init_winconfs() with closedir().";

    DIR * dp = opendir("config/windows");
    exit_err(NULL == dp, world, err_o);
    struct dirent * fn;
    errno = 0;
    char * winconf_ids = try_malloc(256, world, f_name);
    uint8_t i = 0;
    char id;
    while (NULL != (fn = readdir(dp)))
    {
        if (   5 == strlen(fn->d_name)
            && fn->d_name == strstr(fn->d_name, "Win_"))
        {
            id = fn->d_name[4];
            winconf_ids[i] = id;
            i++;
        }
    }
    winconf_ids[i] = '\0';
    exit_err(errno, world, err_r);
    exit_err(closedir(dp), world, err_c);
    world->winconf_ids = try_malloc(strlen(winconf_ids) + 1, world, f_name);
    memcpy(world->winconf_ids, winconf_ids, strlen(winconf_ids) + 1);
    free(winconf_ids);

    struct WinConf * winconfs;
    winconfs = try_malloc(strlen(world->winconf_ids) * sizeof(struct WinConf),
                                 world, f_name);
    i = 0;
    while (0 != (id = get_next_winconf_id(world)))
    {
        create_winconf(world, id, &winconfs[i]);
        i++;
    }
    world->winconfs = winconfs;
    while (0 != (id = get_next_winconf_id(world)))
    {
        init_winconf_from_file(world, id);
        i++;
    }
}



extern void free_winconfs(struct World * world)
{
    char id;
    while (0 != (id = get_next_winconf_id(world)))
    {
        free_winconf_data(world, id);
    }
    free(world->winconf_ids);
    free(world->winconfs);
}



extern void init_wins(struct World * world)
{
    char id;
    while (0 != (id = get_next_winconf_id(world)))
    {
        init_win_from_winconf(world, id);
    }
}



extern void sorted_wintoggle_and_activate(struct World * world)
{
    char * f_name = "sorted_wintoggle_and_activate()";

    char * path = "config/windows/toggle_order_and_active";
    FILE * file = try_fopen(path, "r", world, f_name);
    uint16_t linemax = get_linemax(file, world, f_name);

    char win_order[linemax + 1];
    try_fgets(win_order, linemax + 1, file, world, f_name);

    uint8_t a = 0;
    char * err = trouble_msg(world, f_name, "read_uint8()");
    exit_err(read_uint8(file, &a), world, err);
    free(err);

    try_fclose(file, world, f_name);

    uint8_t i = 0;
    for (; i < linemax - 1; i++)
    {
        if (NULL == strchr(world->winconf_ids, win_order[i]))
        {
            continue;
        }
        struct Win * win = get_win_by_id(world, win_order[i]);
        toggle_window(world->wmeta, win);

        if (a == (uint8_t) win_order[i])
        {
            world->wmeta->active = win;
        }
    }
}



extern void save_win_configs(struct World * world)
{
    char * f_name = "save_win_configs()";

    char id;
    while (0 != (id = get_next_winconf_id(world)))
    {
        save_win_config(world, id);
    }

    char * path     = "config/windows/toggle_order_and_active";
    char * path_tmp = "config/windows/toggle_order_and_active_tmp";
    FILE * file = try_fopen(path_tmp, "w", world, f_name);

    char line[6];
    struct Win * w_p = world->wmeta->chain_start;
    uint8_t i = 0;
    while (0 != w_p)
    {
        struct WinConf * wc = get_winconf_by_win(world, w_p);
        line[i] = wc->id;
        w_p = w_p->next;
        i++;
    }
    line[i] = '\n';
    try_fwrite(line, sizeof(char), strlen(line), file, world, f_name);
    if (0 != world->wmeta->active)
    {
        struct WinConf * wc = get_winconf_by_win(world, world->wmeta->active);
        write_uint8(wc->id, file);
    }

    try_fclose_unlink_rename(file, path_tmp, path, world, f_name);
}



extern uint8_t toggle_window(struct WinMeta * win_meta, struct Win * win)
{
    if (0 != win->frame.curses_win)
    {
        return suspend_win(win_meta, win);
    }
    else
    {
        return append_win(win_meta, win);
    }
}



extern void toggle_winconfig(struct World * world, struct Win * win)
{
    struct WinConf * wcp = get_winconf_by_win(world, win);
    if      (0 == wcp->view)
    {
        win->draw = draw_winconf_geometry;
        wcp->view = 1;
    }
    else if (1 == wcp->view)
    {
        win->draw = draw_winconf_keybindings;
        wcp->view = 2;
    }
    else
    {
        win->draw = get_drawfunc_by_char(wcp->draw);
        wcp->view = 0;
    }
}



extern void toggle_win_height_type(struct World * world, struct Win * win)
{
    struct WinConf * wcp = get_winconf_by_win(world, win);
    if (0 == wcp->height_type)
    {
        wcp->height_type = 1;
    }
    else
    {
        wcp->height_type = 0;
    }
    set_winconf_geometry(world, wcp->id);
}



extern void toggle_win_width_type(struct World * world, struct Win * win)
{
    struct WinConf * wcp = get_winconf_by_win(world, win);
    if (   0 == wcp->width_type
        && win->frame.size.x <= world->wmeta->padframe.size.x)
    {
        wcp->width_type = 1;
    }
    else
    {
        wcp->width_type = 0;
    }
    set_winconf_geometry(world, wcp->id);
}



extern void scroll_pad(struct WinMeta * win_meta, char dir)
{
    if      ('+' == dir)
    {
        reset_pad_offset(win_meta, win_meta->pad_offset + 1);
    }
    else if ('-' == dir)
    {
        reset_pad_offset(win_meta, win_meta->pad_offset - 1);
    }
}



extern uint8_t growshrink_active_window(struct World * world, char change)
{
    if (0 != world->wmeta->active)
    {
        struct yx_uint16 size = world->wmeta->active->frame.size;
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
        uint8_t x = resize_active_win(world->wmeta, size);
        struct WinConf * wcp = get_winconf_by_win(world, world->wmeta->active);
        if (   1 == wcp->width_type
            && world->wmeta->active->frame.size.x
               > world->wmeta->padframe.size.x)
        {
            wcp->width_type = 0;
        }
        set_winconf_geometry(world, wcp->id);
        return x;
    }
    return 0;
}
