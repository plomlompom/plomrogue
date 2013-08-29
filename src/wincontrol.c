/* wincontrol.c */

#include "wincontrol.h"
#include <stdlib.h> /* for malloc(), free() */
#include <string.h> /* for strlen() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include "windows.h" /* for suspend_win(), append_win(), reset_pad_offset(),
                      * resize_active_win(), init_win(), structs Win, WinMeta
                      */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "main.h" /* for Wins struct */
#include "misc.h" /* for textfile_sizes() */
#include "rexit.h" /* for exit_err() */
#include "main.h" /* for World, Wins structs */



extern struct Win init_win_from_file(struct World * world, char * w_name,
                                     void (* f) (struct Win *))
{
    char * err = "Trouble in init_win_from_file() with malloc().";
    char * prefix = "config/Win_";
    uint8_t size = strlen(prefix) + strlen(w_name) + 1;
    char * path = malloc(size);
    exit_err(NULL == path, world, err);
    sprintf(path, "%s%s", prefix, w_name);

    err = "Trouble in init_win_from_file() with fopen().";
    FILE * file = fopen(path, "r");
    free(path);
    exit_err(NULL == file, world, err);
    uint16_t linemax;
    textfile_sizes(file, &linemax, NULL);
    char * line = malloc(linemax);
    err = "Trouble in init_win_from_file() with fgets().";
    exit_err(NULL == fgets(line, linemax, file), world, err);
    int16_t height = atoi(line);
    exit_err(NULL == fgets(line, linemax, file), world, err);
    int16_t width = atoi(line);
    free(line);
    err = "Trouble in init_win_from_file() with fclose().";
    exit_err(fclose(file), world, err);

    struct WinMeta * wmeta = world->wins.meta;
    return init_win(wmeta, w_name, height, width, world, f);
}



extern void sorted_wintoggle(struct World * world)
{
    char * err = "Trouble in sorted_wintoggle() with fopen() on file "
                 "'config/toggle_win_order'.";
    FILE * file = fopen("config/toggle_win_order", "r");
    exit_err(NULL == file, world, err);
    uint16_t linemax;
    textfile_sizes(file, &linemax, NULL);
    char win_order[linemax];
    err = "Trouble in sorted_wintoggle() with fgets() on file "
          "'config/toggle_win_order'.";
    exit_err(NULL == fgets(win_order, linemax, file), world, err);
    err = "Trouble in sorted_wintoggle() with fclose() on file "
          "'toggle_win_order'.";
    exit_err(fclose(file), world, err);

    char c;
    uint8_t i = 0;
    for (; i < linemax; i++)
    {
        c = win_order[i];
        if      ('k' == c)
        {
            toggle_window(world->wins.meta, world->wins.keys);
        }
        else if ('m' == c)
        {
            toggle_window(world->wins.meta, world->wins.map);
        }
        else if ('i' == c)
        {
            toggle_window(world->wins.meta, world->wins.info);
        }
        else if ('l' == c)
        {
            toggle_window(world->wins.meta, world->wins.log);
        }
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

