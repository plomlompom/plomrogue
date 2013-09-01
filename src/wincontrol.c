/* wincontrol.c */

#include "wincontrol.h"
#include <stdlib.h> /* for malloc(), free() */
#include <string.h> /* for strlen() */
#include <stdint.h> /* for uint8_t, uint16_t */
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



/* Initialize Winconf of "id" from appropriate config file.*/
static void init_winconf_from_file(struct World * world, char id);

/* Wrapper around init_win() called with values from appropriate Winconf. */
static void init_win_from_winconf(struct World * world, char id);



static void init_winconf_from_file(struct World * world, char id)
{
    char * err_m = "Trouble in init_win_from_file() with malloc().";
    char * prefix = "config/windows/Win_";
    uint8_t size = strlen(prefix) + 2;
    char * path = malloc(size);
    exit_err(NULL == path, world, err_m);
    sprintf(path, "%s_", prefix);
    path[size - 2] = id;
    char * err = "Trouble in init_win_from_file() with fopen().";
    FILE * file = fopen(path, "r");
    exit_err(NULL == file, world, err);
    free(path);

    struct WinConf * winconf = get_winconf_by_id(world, id);
    uint16_t linemax;
    err = "Trouble in init_win_from_file() with textfile_sizes().";
    exit_err(textfile_sizes(file, &linemax, NULL), world, err);
    char * line = malloc(linemax);
    exit_err(NULL == line, world, err_m);
    err = "Trouble in init_win_from_file() with fgets().";

    exit_err(NULL == fgets(line, linemax, file), world, err);
    winconf->title = malloc(strlen(line));
    exit_err(NULL == winconf->title, world, err_m);
    memcpy(winconf->title, line, strlen(line) - 1); /* Eliminate newline char */
    winconf->title[strlen(line) - 1] = '\0';        /* char at end of string. */
    exit_err(NULL == fgets(line, linemax, file), world, err);
    winconf->height = atoi(line);
    exit_err(NULL == fgets(line, linemax, file), world, err);
    winconf->width = atoi(line);

    free(line);
    err = "Trouble in init_win_from_file() with fclose().";
    exit_err(fclose(file), world, err);
}



static void init_win_from_winconf(struct World * world, char id)
{
    struct WinConf * winconf = get_winconf_by_id(world, id);
    char * err = "Trouble in init_win_from_file() with init_win().";
    exit_err(init_win(world->wmeta, &winconf->win, winconf->title,
                      winconf->height, winconf->width, world, winconf->draw),
             world, err);
}



extern void create_winconfs(struct World * world)
{
    char * err = "Trouble with malloc() in init_winconfs().";
    struct WinConf * winconfs = malloc(4 * sizeof(struct WinConf));
    exit_err(NULL == winconfs, world, err);
    winconfs[0].id = 'i';
    winconfs[0].draw = draw_info_win;
    winconfs[1].id = 'k';
    winconfs[1].draw = draw_keys_win;
    winconfs[2].id = 'l';
    winconfs[2].draw = draw_log_win;
    winconfs[3].id = 'm';
    winconfs[3].draw = draw_map_win;
    world->winconfs = winconfs;
}



extern void init_winconfs(struct World * world)
{
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



extern void reload_win_config(struct World * world)
{
    while (0 != world->wmeta->active)
    {
        suspend_win(world->wmeta, world->wmeta->active);
    }
    free_wins(world);
    free_winconfs(world);
    create_winconfs(world);
    init_winconfs(world);
    init_wins(world);
    sorted_wintoggle(world);
}



extern struct WinConf * get_winconf_by_id(struct World * world, char id)
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



extern struct Win * get_win_by_id(struct World * world, char id)
{
    struct WinConf * wc = get_winconf_by_id(world, id);
    return wc->win;
}



extern void sorted_wintoggle(struct World * world)
{
    char * err = "Trouble in sorted_wintoggle() with fopen().";
    FILE * file = fopen("config/windows/toggle_order", "r");
    exit_err(NULL == file, world, err);
    uint16_t linemax;
    err = "Trouble in sorted_wintoggle() with textfile_sizes().";
    exit_err(textfile_sizes(file, &linemax, NULL), world, err);
    char win_order[linemax];
    err = "Trouble in sorted_wintoggle() with fgets().";
    exit_err(NULL == fgets(win_order, linemax, file), world, err);
    err = "Trouble in sorted_wintoggle() with fclose().";
    exit_err(fclose(file), world, err);
    uint8_t i = 0;
    for (; i < linemax - 2; i++)
    {
        toggle_window(world->wmeta, get_win_by_id(world, win_order[i]));
    }
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



extern uint8_t growshrink_active_window(struct WinMeta * win_meta, char change)
{
    if (0 != win_meta->active)
    {
        struct yx_uint16 size = win_meta->active->frame.size;
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
        return resize_active_win (win_meta, size);
    }
    return 0;
}
