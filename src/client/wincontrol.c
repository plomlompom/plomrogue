/* src/client/wincontrol.c */

#include "wincontrol.h"
#include <ncurses.h> /* getmaxx(), getmaxy(), wresize() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint32_t, UINT16_MAX */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* memcpy(), memset(), strchr(), strlen() */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "windows.h" /* Win,yx_uint16, get_win_by_id(),get_win_pos_in_order() */
#include "world.h" /* global world */



/* Get Win before window identified by "c" or NULL if there is none. */
static struct Win * get_win_before(char c);

/* Make .v_screen just wide enough to contain all visible windows. */
static void refit_v_screen();

/* Update geometry (sizes, positions) of window "w" and its successors in the
 * window chain. Use place_win() for the positioning algorithm.
 */
static void update_wins(struct Win * w);
static void place_win(struct Win * w);

/* Write "win"'s size back to .target_(height/width) as per .target_*_type. */
static void set_win_target_size(struct Win * win);

/* Append/suspend window "w" to/from chain of visible windows. Appended windows
 * will become active. Suspended active windows will move the active window
 * selection to their successor in the window chain or, failing that, their
 * predecessor, or, failing that, to 0 (no window active).
 */
static void append_win(struct Win * w);
static void suspend_win(struct Win * w);



extern struct Win * get_win_before(char c)
{
    uint8_t i = get_win_pos_in_order(c);
    if (i > 0)
    {
        return get_win_by_id(world.winDB.order[i - 1]);
    }
    return NULL;
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
    if ((uint32_t) getmaxx(world.winDB.v_screen) + 1 != lastwcol)
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
    uint8_t sep = 1;         /* Width of inter-window borders and title bars. */

    /* If w is first window, it goes into the top left corner. */
    w->start.x = 0;
    w->start.y = 0 + sep;
    struct Win * w_prev = get_win_before(w->id);
    if (w_prev)
    {

        /* If not, get w's next predecessor starting a new stack on the screen
         * top, fit w's top left corner to that predecessor's top right corner.
         */
        struct Win * w_top = w_prev;
        for (; w_top->start.y != 0 + sep; w_top = get_win_before(w_top->id));
        w->start.x = w_top->start.x + w_top->frame_size.x + sep;

        /* If enough space is found below w's predecessor, fit w's top left
         * corner to that predecessor's bottom left corner.
         */
        uint16_t next_free_y = w_prev->start.y + w_prev->frame_size.y + sep;
        if (   w->frame_size.x <= w_prev->frame_size.x
            && w->frame_size.y <= world.winDB.v_screen_size.y - next_free_y)
        {
            w->start.x = w_prev->start.x;
            w->start.y = next_free_y;
            return;
        }

        /* If that fails, try to fit w's top left corner to the top right corner
         * of its next predecessor w_test 1) below w_top (w's next predecessor
         * starting a new stack on the screen top) 2) and the most rightward
         * lower neighbor of a window w_high, itself throning over enough free
         * space for w to fit below it, rightwards of its lower neighbor w_test.
         */
        struct Win * w_test = w_prev;
        struct Win * w_high;
        while (w_test != w_top)
        {
            for (w_high = get_win_before(w_test->id);  /* Walk down chain     */
                 w_test->start.y <= w_high->start.y;   /* until w_high starts */
                 w_high = get_win_before(w_high->id)); /* higher than w_test. */
            next_free_y = w_high->start.y + w_high->frame_size.y + sep;
            uint16_t first_free_x = w_test->start.x + w_test->frame_size.x +sep;
            uint16_t last_free_x = w_high->start.x + w_high->frame_size.x;
            if (   w->frame_size.y <= world.winDB.v_screen_size.y - next_free_y
                && w->frame_size.x <= last_free_x - first_free_x)
            {
                w->start.x = first_free_x;
                w->start.y = next_free_y;
                break;
            }
            w_test = w_high;
        }
    }
}



static void set_win_target_size(struct Win * w)
{
    if      (0 == w->target_height_type)
    {
        w->target_height = w->frame_size.y;
    }
    else if (1 == w->target_height_type)
    {
        w->target_height = w->frame_size.y - world.winDB.v_screen_size.y +1;
    }
    if      (0 == w->target_width_type)
    {
        w->target_width = w->frame_size.x;
    }
    else if (1 == w->target_width_type)
    {
        w->target_width = w->frame_size.x - world.winDB.v_screen_size.x;
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
    uint8_t i = get_win_pos_in_order(w->id);
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



extern void toggle_linebreak_type()
{
    struct Win * w = get_win_by_id(world.winDB.active);
    if      (0 == w->linebreak)
    {
        w->linebreak = 1;
    }
    else if (1 == w->linebreak)
    {
        w->linebreak = 2;
    }
    else if (2 == w->linebreak)
    {
        w->linebreak = 0;
    }
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
        else if (change == '_' && w->frame_size.x > 1)
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
        uint8_t pos = get_win_pos_in_order(world.winDB.active);
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
        uint8_t pos = get_win_pos_in_order(world.winDB.active);
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
