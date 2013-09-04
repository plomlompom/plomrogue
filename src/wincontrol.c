/* wincontrol.c */

#include "wincontrol.h"
#include <stdlib.h> /* for malloc(), free() */
#include <string.h> /* for strlen() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include <stdio.h> /* for fopen(), fclose(), fwrite() */
#include <unistd.h> /* for access(), unlink() */
#include "windows.h" /* for suspend_win(), append_win(), reset_pad_offset(),
                      * resize_active_win(), init_win(), free_win(),
                      * structs Win, WinMeta
                      */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "main.h" /* for Wins struct */
#include "misc.h" /* for textfile_sizes() */
#include "rexit.h" /* for exit_err() */
#include "main.h" /* for World, Wins structs */
#include "draw_wins.h" /* for draw_keys_win(), draw_info_win(), draw_log_win(),
                        * draw_map_win
                        */



/* Return string "prefix" + "id"; malloc()'s string, remember to call free()! */
static char * string_prefixed_id(struct World * world, char * prefix, char id);



/* Create Winconf, initialize ->iew/height_type/width_type to 0, ->id to "id"
 * and ->draw to f.
 */
static void create_winconf(char id, struct WinConf * wcp,
                           void (* f) (struct Win *));

/* Initialize Winconf of "id" from appropriate config file.*/
static void init_winconf_from_file(struct World * world, char id);

/* Wrapper around init_win() called with values from Winconf of "id". */
static void init_win_from_winconf(struct World * world, char id);

/* Save title, size of window identified by "id" to its configuration file. */
static void save_win_config(struct World * world, char id);



/* Write size of a window to its WinConf, as positive or negative values
 * (dependent on state ofWinConf->height_type / WinConf->width_type).
 */
static void set_winconf(struct World * world, char id);



/* Get WinConf by "id"; get id of WinConf mothering "win". */
static struct WinConf * get_winconf_by_id(struct World * world, char id);
static char get_id_by_win(struct World * world, struct Win * win);



static char * string_prefixed_id(struct World * world, char * prefix, char id)
{
    char * err = "Trouble in string_prefixed_id() with malloc().";
    uint8_t size = strlen(prefix) + 2;
    char * path = malloc(size);
    exit_err(NULL == path, world, err);
    sprintf(path, "%s_", prefix);
    path[size - 2] = id;
    return path;
}



static void create_winconf(char id, struct WinConf * wcp,
                           void (* f) (struct Win *))
{
    wcp->id = id;
    wcp->draw = f;
    wcp->view = 0;
    wcp->height_type = 0;
    wcp->width_type = 0;
}



static void init_winconf_from_file(struct World * world, char id)
{
    char * err_o = "Trouble in init_win_from_file() with fopen().";
    char * err_m = "Trouble in init_win_from_file() with malloc().";
    char * err_s = "Trouble in init_win_from_file() with textfile_sizes().";
    char * err_g = "Trouble in init_win_from_file() with fgets().";
    char * err_c = "Trouble in init_win_from_file() with fclose().";

    char * path = string_prefixed_id(world, "config/windows/Win_", id);
    FILE * file = fopen(path, "r");
    exit_err(NULL == file, world, err_o);
    free(path);

    uint16_t linemax;
    exit_err(textfile_sizes(file, &linemax, NULL), world, err_s);
    char * line = malloc(linemax);
    exit_err(NULL == line, world, err_m);

    struct WinConf * winconf = get_winconf_by_id(world, id);
    exit_err(NULL == fgets(line, linemax, file), world, err_g);
    exit_err(NULL == (winconf->title = malloc(strlen(line))), world, err_m);
    memcpy(winconf->title, line, strlen(line) - 1); /* Eliminate newline char */
    winconf->title[strlen(line) - 1] = '\0';        /* char at end of string. */
    exit_err(NULL == fgets(line, linemax, file), world, err_g);
    winconf->height = atoi(line);
    if (0 >= winconf->height)
    {
        winconf->height_type = 1;
    }
    exit_err(NULL == fgets(line, linemax, file), world, err_g);
    winconf->width = atoi(line);
    if (0 >= winconf->width)
    {
        winconf->width_type = 1;
    }

    free(line);
    exit_err(fclose(file), world, err_c);
}



static void init_win_from_winconf(struct World * world, char id)
{
    char * err = "Trouble in init_win_from_file() with init_win().";
    struct WinConf * winconf = get_winconf_by_id(world, id);
    exit_err(init_win(world->wmeta, &winconf->win, winconf->title,
                      winconf->height, winconf->width, world, winconf->draw),
             world, err);
}



extern void save_win_config(struct World * world, char id)
{
    char * err_o = "Trouble in save_win_config() with fopen().";
    char * err_m = "Trouble in save_win_config() with malloc().";
    char * err_c = "Trouble in save_win_config() with fclose().";
    char * err_u = "Trouble in save_win_config() with unlink().";
    char * err_r = "Trouble in save_win_config() with rename().";

    char * path_tmp = string_prefixed_id(world, "config/windows/Win_tmp_", id);
    FILE * file = fopen(path_tmp, "w");
    exit_err(NULL == file, world, err_o);

    struct WinConf * wc = get_winconf_by_id(world, id);
    uint8_t size = strlen(wc->title) + 2;
    if (size < 7)
    {
        size = 7;
    }
    char * line = malloc(size);
    exit_err(NULL == line, world, err_m);
    sprintf(line, "%s\n", wc->title);
    fwrite(line, sizeof(char), strlen(line), file);
    sprintf(line, "%d\n", wc->height);
    fwrite(line, sizeof(char), strlen(line), file);
    sprintf(line, "%d\n", wc->width);
    fwrite(line, sizeof(char), strlen(line), file);

    exit_err(fclose(file), world, err_c);
    char * path = string_prefixed_id(world, "config/windows/Win_", id);
    if (!access(path, F_OK))
    {
        exit_err(unlink(path), world, err_u);
    }
    exit_err(rename(path_tmp, path), world, err_r);
    free(path);
    free(path_tmp);
}



static void set_winconf(struct World * world, char id)
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



static char get_id_by_win(struct World * world, struct Win * win)
{
    struct WinConf * wc = get_winconf_by_win(world, win);
    return wc->id;
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
    char * err = "Trouble with malloc() in init_winconfs().";
    struct WinConf * winconfs = malloc(4 * sizeof(struct WinConf));
    exit_err(NULL == winconfs, world, err);
    create_winconf('i', &winconfs[0], draw_info_win);
    create_winconf('k', &winconfs[1], draw_keys_win);
    create_winconf('l', &winconfs[2], draw_log_win);
    create_winconf('m', &winconfs[3], draw_map_win);
    world->winconfs = winconfs;
    init_winconf_from_file(world, 'i');
    init_winconf_from_file(world, 'k');
    init_winconf_from_file(world, 'l');
    init_winconf_from_file(world, 'm');
}



extern void free_winconf(struct World * world, char id)
{
    struct WinConf * wc = get_winconf_by_id(world, id);
    free(wc->title);
}



extern void free_winconfs(struct World * world)
{
    free_winconf(world, 'i');
    free_winconf(world, 'k');
    free_winconf(world, 'l');
    free_winconf(world, 'm');
    free(world->winconfs);
}



extern void init_wins(struct World * world)
{
    init_win_from_winconf(world, 'i');
    init_win_from_winconf(world, 'k');
    init_win_from_winconf(world, 'l');
    init_win_from_winconf(world, 'm');
}



extern void free_wins(struct World * world)
{
    free_win(get_win_by_id(world, 'i'));
    free_win(get_win_by_id(world, 'k'));
    free_win(get_win_by_id(world, 'l'));
    free_win(get_win_by_id(world, 'm'));
}



extern void sorted_wintoggle(struct World * world)
{
    char * err_o = "Trouble in sorted_wintoggle() with fopen().";
    char * err_s = "Trouble in sorted_wintoggle() with textfile_sizes().";
    char * err_g = "Trouble in sorted_wintoggle() with fgets().";
    char * err_c = "Trouble in sorted_wintoggle() with fclose().";

    char * path = "config/windows/toggle_order";
    FILE * file = fopen(path, "r");
    exit_err(NULL == file, world, err_o);

    uint16_t linemax;
    exit_err(textfile_sizes(file, &linemax, NULL), world, err_s);

    char win_order[linemax];
    exit_err(NULL == fgets(win_order, linemax, file), world, err_g);
    exit_err(fclose(file), world, err_c);

    uint8_t i = 0;
    for (; i < linemax - 2; i++)
    {
        toggle_window(world->wmeta, get_win_by_id(world, win_order[i]));
    }
}



extern void reload_win_config(struct World * world)
{
    while (0 != world->wmeta->active)
    {
        suspend_win(world->wmeta, world->wmeta->active);
    }
    free_wins(world);
    free_winconfs(world);
    init_winconfs(world);
    init_wins(world);
    sorted_wintoggle(world);
}



extern void save_win_configs(struct World * world)
{
    save_win_config(world, 'i');
    save_win_config(world, 'k');
    save_win_config(world, 'l');
    save_win_config(world, 'm');

    char * err_o = "Trouble in save_win_configs() with fopen().";
    char * err_m = "Trouble in save_win_configs() with calloc().";
    char * err_c = "Trouble in save_win_configs() with fclose().";
    char * err_u = "Trouble in save_win_configs() with unlink().";
    char * err_r = "Trouble in save_win_configs() with rename().";

    char * path     = "config/windows/toggle_order";
    char * path_tmp = "config/windows/toggle_order_tmp";
    FILE * file = fopen(path_tmp, "w");
    exit_err(NULL == file, world, err_o);

    char * line = calloc(6, sizeof(char));
    exit_err(NULL == line, world, err_m);
    struct Win * w_p = world->wmeta->_chain_start;
    uint8_t i = 0;
    while (0 != w_p)
    {
        line[i] = get_id_by_win(world, w_p);
        w_p = w_p->_next;
        i++;
    }
    line[i] = '\n';
    fwrite(line, sizeof(char), strlen(line), file);

    exit_err(fclose(file), world, err_c);
    if (!access(path, F_OK))
    {
        exit_err(unlink(path), world, err_u);
    }
    exit_err(rename(path_tmp, path), world, err_r);
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
    if (0 == wcp->view)
    {
        win->_draw = draw_winconf;
        wcp->view = 1;
    }
    else
    {
        win->_draw = wcp->draw;
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
    set_winconf(world, wcp->id);
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
    set_winconf(world, wcp->id);
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
        set_winconf(world, wcp->id);
        return x;
    }
    return 0;
}
