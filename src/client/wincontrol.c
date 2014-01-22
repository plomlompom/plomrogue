/* src/client/wincontrol.c */

#include "wincontrol.h"
#include <errno.h> /* global errno */
#include <dirent.h> /* DIR, struct dirent, opendir(), closedir(), readdir() */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* strlen(), strchr(), strstr(), memcpy() */
#include "../common/readwrite.h" /* try_fopen(), textfile_sizes(), try_fgets(),
                                  * textfile_sizes(), try_fwrite(), try_fgetc(),
                                  * try_fclose_unlink_rename(), try_fputc()
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



/* Return string "prefix" + "id"; malloc()'s string, remember to call free()! */
static char * string_prefixed_id(char * prefix, char id);

/* Initialize Winconf of "id" from appropriate config file.*/
static void init_winconf_from_file(char id, struct WinConf * winconf);

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

/* Iterate over chars of world.winconf_db.winconf_ids array. Restart after \0.*/
static char get_next_winconf_id();



static char * string_prefixed_id(char * prefix, char id)
{
    uint8_t size = strlen(prefix) + 2;
    char * path = try_malloc(size, "string_prefixed_id()");
    sprintf(path, "%s_", prefix);
    path[size - 2] = id;
    return path;
}



static void init_winconf_from_file(char id, struct WinConf * winconf)
{
    /* Assign WinConf id to filename path, error message context, winconf->id.*/
    char * tmp = "init_winconf_from_file() on window id '_'";
    char * context = try_malloc(strlen(tmp) + 1, "init_winconf_from_file()");
    memcpy(context, tmp, strlen(tmp) + 1);
    context[strlen(tmp) - 2] = id;
    char * path = string_prefixed_id("confclient/windows/Win_", id);
    winconf->id = id;

    /* Prepare reading in file line by line into "line" array. */
    FILE * file = try_fopen(path, "r", context);
    free(path);
    uint32_t linemax = textfile_sizes(file, NULL);
    char line[linemax + 1];

    /* Read/determine winconf->title, ->draw, ->height(_type),->width(_type). */
    try_fgets(line, linemax + 1, file, context);
    winconf->title = try_malloc(strlen(line), context);
    memcpy(winconf->title, line, strlen(line) - 1); /* Eliminate newline char */
    winconf->title[strlen(line) - 1] = '\0';        /* char at end of string. */
    try_fgets(line, linemax + 1, file, context);
    winconf->draw = line[0];
    try_fgets(line, linemax + 1, file, context);
    winconf->height = atoi(line);
    winconf->height_type = (0 >= winconf->height);
    try_fgets(line, linemax + 1, file, context);
    winconf->width = atoi(line);
    winconf->width_type = (0 >= winconf->width);

    /* Read in window-specific keybindings (winconf->kb). */
    char command[linemax + 1];
    char * cmdptr;
    struct KeyBinding ** loc_last_ptr = &winconf->kb.kbs;
    * loc_last_ptr = 0;
    while (try_fgets(command, linemax + 1, file, context))
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
        cmdptr[strlen(cmdptr) - 1] = '\0';
        kb_p->command = get_command(cmdptr);
        loc_last_ptr = & kb_p->next;
    }

    /* Init remaining values to zero and cleaning up. */
    winconf->view = 0;
    winconf->kb.edit = 0;
    winconf->kb.select = 0;
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

    /* Prepare atomic file saving. */
    char * path_tmp = string_prefixed_id("confclient/windows/Win_tmp_", id);
    FILE * file = try_fopen(path_tmp, "w", f_name);

    /* Save, line by line, ->title, ->draw, ->height and ->width. */
    struct WinConf * wc = get_winconf_by_id(id);
    uint8_t size = strlen(wc->title) + 2;
    if (size < 7)  /* Ensure that at least 5 + 2 char fit into line so that   */
    {              /* the digit representation of any uint16_t may be stored. */
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

    /* Save window-specific keybindings (->kb.kbs). */
    uint16_t linemax = 0;
    struct KeyBinding * kb_p = wc->kb.kbs;
    while (0 != kb_p)
    {
        if (strlen(kb_p->command->dsc_short) > linemax)
        {
            linemax = strlen(kb_p->command->dsc_short);
        }
        kb_p = kb_p->next;
    }
    linemax = linemax + 6;          /* + 6: + 3 digits + whitespace + \n + \0 */
    char kb_line[linemax];
    kb_p = wc->kb.kbs;
    while (0 != kb_p)
    {
        sprintf(kb_line, "%d %s\n", kb_p->key, kb_p->command->dsc_short);
        try_fwrite(kb_line, sizeof(char), strlen(kb_line), file, f_name);
        kb_p = kb_p->next;
    }

    /* Finish atomic file saving and clean up. */
    char * path = string_prefixed_id("confclient/windows/Win_", id);
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
        wcp->height = wcp->win->framesize.y - world.wmeta.padsize.y + 1;
    }
    if      (0 == wcp->width_type)
    {
        wcp->width = wcp->win->framesize.x;
    }
    else if (1 == wcp->width_type)
    {
        wcp->width = wcp->win->framesize.x - world.wmeta.padsize.x;
    }
}



static struct WinConf * get_winconf_by_id(char id)
{
    uint8_t i = 0;
    while (1)
    {
        if (id == world.winconf_db.winconfs[i].id)
        {
            return &world.winconf_db.winconfs[i];
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
    char c = world.winconf_db.winconf_ids[i];
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
        if (win == world.winconf_db.winconfs[i].win)
        {
            return &world.winconf_db.winconfs[i];
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

    /* Fill world.winconf_db.winconf_ids with confclient/windows/Win_*
     * filenames' end chars.
     */
    uint8_t max_wins = 255;         /* Maximum number of window ids to store. */
    DIR * dp = opendir("confclient/windows");
    exit_trouble(NULL == dp, f_name, "opendir()");
    struct dirent * fn;
    errno = 0;
    char * winconf_ids = try_malloc(max_wins + 1, f_name);
    uint8_t i = 0;
    char id;
    while (NULL != (fn = readdir(dp)) && i < max_wins)
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
    world.winconf_db.winconf_ids = try_malloc(strlen(winconf_ids) + 1, f_name);
    memcpy(world.winconf_db.winconf_ids, winconf_ids, strlen(winconf_ids) + 1);
    free(winconf_ids);

    /* Initialize world.winconf_db.winconfs from Win_* files named in
     * world.winconf_db.winconf_ids.
     */
    size_t size = strlen(world.winconf_db.winconf_ids) * sizeof(struct WinConf);
    world.winconf_db.winconfs = try_malloc(size, f_name);
    i = 0;
    while (0 != (id = get_next_winconf_id()))
    {
        init_winconf_from_file(id, &world.winconf_db.winconfs[i]);
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
    free(world.winconf_db.winconf_ids);
    free(world.winconf_db.winconfs);
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

    /* Read from file order of windows to be toggled + active win selection. */
    char * path = "confclient/windows/toggle_order_and_active";
    FILE * file = try_fopen(path, "r", f_name);
    uint32_t linemax = textfile_sizes(file, NULL);
    char win_order[linemax + 1];
    try_fgets(win_order, linemax + 1, file, f_name);
    int char_or_eof = try_fgetc(file, f_name);
    exit_trouble(EOF==char_or_eof, f_name, "fgetc() unexpectedly hitting EOF");
    uint8_t a = (uint8_t) char_or_eof;
    try_fclose(file, f_name);

    /* Toggle windows and set active window selection. */
    uint8_t i = 0;
    for (; i < strlen(win_order) - 1; i++)
    {
        if (NULL == strchr(world.winconf_db.winconf_ids, win_order[i]))
        {
            continue;
        }
        toggle_window(win_order[i]);
        if (a == (uint8_t) win_order[i])
        {
            world.wmeta.active = get_win_by_id(win_order[i]);
        }
    }
}



extern void save_win_configs()
{
    char * f_name = "save_win_configs()";

    /* Save individual world.winconf_db.winconfs to their proper files. */
    uint8_t max_wins = 255;     /* n of WinConf fitting into world.winconf_db */
    char id;
    while (0 != (id = get_next_winconf_id()))
    {
        save_win_config(id);
    }

    /* Save order of windows to toggle on start / which to select as active. */
    char * path     = "confclient/windows/toggle_order_and_active";
    char * path_tmp = "confclient/windows/toggle_order_and_active_tmp";
    FILE * file = try_fopen(path_tmp, "w", f_name);
    char line[max_wins + 2];
    struct Win * w_p = world.wmeta.chain_start;
    uint8_t i = 0;
    while (0 != w_p && i < max_wins)
    {
        struct WinConf * wc = get_winconf_by_win(w_p);
        line[i] = wc->id;
        w_p = w_p->next;
        i++;
    }
    line[i] = '\n';
    line[i + 1] = '\0';
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    if (0 != world.wmeta.active)
    {
        struct WinConf * wc = get_winconf_by_win(world.wmeta.active);
        try_fputc(wc->id, file, f_name);
    }
    try_fclose_unlink_rename(file, path_tmp, path, f_name);
}



extern void toggle_window(char id)
{
    struct Win * win = get_win_by_id(id);
    if (0 == win->prev && world.wmeta.chain_start != win)   /* Win struct is  */
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
   struct Win * win = world.wmeta.active;
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
        win->draw     = get_drawfunc_by_char(wcp->draw);
        win->center   = wcp->center;
    }
}



extern void toggle_win_size_type(char axis)
{
    struct Win * win = world.wmeta.active;
    struct WinConf * wcp = get_winconf_by_win(win);
    if ('y' == axis)
    {
        wcp->height_type = (0 == wcp->height_type);
        set_winconf_geometry(wcp->id);
        return;
    }
    wcp->width_type = (   0 == wcp->width_type
                       && win->framesize.x <= world.wmeta.padsize.x);
    set_winconf_geometry(wcp->id);
}



extern void scroll_pad(char dir)
{
    if      ('+' == dir)
    {
        reset_pad_offset(world.wmeta.pad_offset + 1);
    }
    else if ('-' == dir)
    {
        reset_pad_offset(world.wmeta.pad_offset - 1);
    }
}



extern void growshrink_active_window(char change)
{
    if (0 != world.wmeta.active)
    {
        struct yx_uint16 size = world.wmeta.active->framesize;
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
        struct WinConf * wcp = get_winconf_by_win(world.wmeta.active);
        if (   1 == wcp->width_type
            && world.wmeta.active->framesize.x > world.wmeta.padsize.x)
        {
            wcp->width_type = 0;
        }
        set_winconf_geometry(wcp->id);
    }
}
