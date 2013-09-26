/* draw_wins.c */

#include "draw_wins.h"
#include <stdlib.h>      /* for free() */
#include <stdint.h>      /* for uint16_t */
#include <string.h>      /* for strlen() */
#include <ncurses.h>     /* for mvwaddch() */
#include "windows.h"     /* for structs Win, Frame, for draw_scroll_hint() */
#include "misc.h"        /* for center_offset(), try_malloc() */
#include "keybindings.h" /* for struct KeyBinding, for get_name_to_keycode() */
#include "map_objects.h" /* for structs MapObj, get_map_object_def(),
                          * get_player()
                          */
#include "map.h"         /* for Map struct */
#include "main.h"        /* for World struct */
#include "rexit.h"       /* for err_exit() */
#include "command_db.h"  /* for get_command_longdesc() */
#include "wincontrol.h"  /* for WinConf struct, get_winconf_by_win() */



/* Write "text" into window "win" as far as possible. Start on row "start_y".
 * Break lines at newlines.
 */
static void draw_with_linebreaks(struct Win * win, char * text,
                                 uint16_t start_y);

/* Write "line" into window "win" at line "y" as far as it fits into it; apply
 * ncurses attribute "attri" to all characters drawn; if "fill" is non-zero,
 * fill the entire line with empty characters ("attri" also applied on these).
 */
static void draw_line(struct Win * win, uint16_t y, char * line, attr_t attri,
                      uint8_t fill);

/* Write "text" not starting from the top but from the bottom of "win". */
static void draw_text_from_bottom(struct Win * win, char * text);

/* Draw onto "map" in "win" the objects in the chain at "start". */
static void draw_map_objects(struct World * world, struct MapObj * start,
                             struct Map * map, struct Win * win);

/* Return keybinding list line via "kb_pp", iterate pointer pointed to by it. */
static char * get_kb_line_and_iterate(struct World * world,
                                      struct KeyBinding ** kb_pp);

/* Draw from line "start" on config view for keybindings defined at "kb". */
static void draw_kb_view(struct World * world, struct Win * win,
                         char * f_name, struct KeyBiData * kb, uint8_t start);

/* Draw into window "win" from line "start" on a "title" followed by an empty
 * line followed by a list of all keybindings starting at kb_p.
 */
static uint16_t draw_titled_keybinding_list(struct World * world,
                                            struct Win * win, uint16_t start,
                                            char * title,
                                            struct KeyBinding * kb_p);






static void draw_with_linebreaks(struct Win * win, char * text,
                                 uint16_t start_y)
{
    uint16_t x, y;
    char toggle;
    char fin = 0;
    int16_t z = -1;
    for (y = start_y; y < win->frame.size.y; y++)
    {
        if (0 == fin)
        {
            toggle = 0;
        }
        for (x = 0; x < win->frame.size.x; x++)
        {
            if (0 == toggle)
            {
                z++;
                if ('\n' == text[z])
                {
                    toggle = 1;
                    continue;
                }
                else
                {
                    mvwaddch(win->frame.curses_win, y, x, text[z]);
                }
                if ('\n' == text[z+1])
                {
                    z++;
                    toggle = 1;
                }
                else if (0 == text[z+1])
                {
                    toggle = 1;
                    fin = 1;
                }
            }
        }
    }
}



static void draw_line(struct Win * win, uint16_t y, char * line, attr_t attri,
                      uint8_t fill)
{
    uint16_t x = 0;
    for (; x < win->frame.size.x && x < strlen(line); x++)
    {
        mvwaddch(win->frame.curses_win, y, x, line[x] | attri);
    }
    if (0 != fill)
    {
        for (; x < win->frame.size.x; x++)
        {
            mvwaddch(win->frame.curses_win, y, x, ' ' | attri);
        }
    }
}



static void draw_text_from_bottom (struct Win * win, char * text)
{
    /* Determine number of lines text would have in a window of win's width,
     * but infinite height. Treat \n and \0 as control chars for incrementing
     * y and stopping the loop. Make sure +they* don't count as cell space.
     */
    char toggle = 0;
    uint16_t x, y, offset;
    int16_t z = -1;
    for (y = 0; 0 == toggle; y++)
    {
        for (x = 0; x < win->frame.size.x; x++)
        {
            z++;
            if ('\n' == text[z])
            {
                break;
            }
            if ('\n' == text[z+1])
            {
                z++;
                break;
            }
            else if (0 == text[z+1])
            {
                toggle = 1;
                break;
            }
        }
    }
    z = -1;

    /* Depending on what's bigger, determine start point in window or text. */
    uint16_t start_y = 0;
    if (y < win->frame.size.y)
    {
        start_y = win->frame.size.y - y;
    }
    else if (y > win->frame.size.y)
    {
        offset = y - win->frame.size.y;
        for (y = 0; y < offset; y++)
        {
            for (x = 0; x < win->frame.size.x; x++)
            {
                z++;
                if ('\n' == text[z])
                {
                    break;
                }
                if ('\n' == text[z+1])
                {
                    z++;
                    break;
                }
            }
        }
        text = text + (sizeof(char) * (z + 1));
    }

    draw_with_linebreaks(win, text, start_y);
}



static void draw_map_objects(struct World * world, struct MapObj * start,
                             struct Map * map, struct Win * win)
{
    struct MapObj * o;
    struct MapObjDef * d;
    char c;
    uint8_t i;
    for (i = 0; i < 2; i++)
    {
        for (o = start; o != 0; o = o->next)
        {
            if (   (   (0 == i && 0 == o->lifepoints)      /* Draw in-animate */
                    || (1 == i && 0 < o->lifepoints))      /* objects first.  */
                && o->pos.y >= map->offset.y
                && o->pos.y <  map->offset.y + win->frame.size.y
                && o->pos.x >= map->offset.x
                && o->pos.x <  map->offset.x + win->frame.size.x)
            {
                d = get_map_object_def(world, o->type);
                c = d->char_on_map;
                mvwaddch(win->frame.curses_win,
                         o->pos.y - map->offset.y, o->pos.x - map->offset.x, c);
            }
        }
    }
}



static char * get_kb_line_and_iterate(struct World * world,
                                      struct KeyBinding ** kb_pp)
{
    char * f_name = "get_kb_line_and_iterate()";
    struct KeyBinding * kb_p = * kb_pp;
    char * keyname = get_name_to_keycode(world, kb_p->key);
    char * cmd_dsc = get_command_longdsc(world, kb_p->name);
    uint16_t size = 9 + 1 + strlen(cmd_dsc) + 1;
    char * line = try_malloc(size, world, f_name);
    sprintf(line, "%-9s %s", keyname, cmd_dsc);
    free(keyname);
    * kb_pp = kb_p->next;
    return line;
}



static void draw_kb_view(struct World * world, struct Win * win,
                         char * f_name, struct KeyBiData * kb, uint8_t start)
{
    if (0 == kb->kbs)
    {
        draw_line(win, start, "(none)", 0, 0);
    }
    char * err_hint = trouble_msg(world, f_name, "draw_scroll_hint()");
    uint16_t kb_max = get_n_of_keybs(kb->kbs) - 1;
    uint16_t y, offset;
    offset = center_offset(kb->select, kb_max, win->frame.size.y - 1 - start);
    uint16_t y_border = win->frame.size.y + offset - 1 - start;
    struct KeyBinding * kb_p = get_keyb_of_n(kb->kbs, offset + (offset > 0));
    for (y = start; 0 != kb_p && y < win->frame.size.y; y++)
    {
        if (start == y && offset > 0)
        {
            uint8_t test = draw_scroll_hint(&win->frame, y, offset + 1, '^');
            exit_err(test, world, err_hint);
            continue;
        }
        else if (win->frame.size.y == y + 1 && kb_max > y_border)
        {
            uint16_t pos = kb_max - (offset + win->frame.size.y) + 2 + start;
            uint8_t test = draw_scroll_hint(&win->frame, y, pos, 'v');
            exit_err(test, world, err_hint);
            continue;
        }
        attr_t attri = 0;
        if (y - start == kb->select - offset)
        {
            attri = A_REVERSE;
            if (1 == kb->edit)
            {
                attri = attri | A_BLINK;
            }
        }
        char * kb_line = get_kb_line_and_iterate(world, &kb_p);
        draw_line(win, y, kb_line, attri, 1);
        free(kb_line);
    }
    free(err_hint);
}



static uint16_t draw_titled_keybinding_list(struct World * world,
                                            struct Win * win, uint16_t start,
                                            char * title,
                                            struct KeyBinding * kb_p)
{
    uint16_t x, y;
    uint16_t i = 0;
    uint8_t state = 0;
    for (y = start; y < win->frame.size.y && (0 == state || 0 != kb_p); y++)
    {
        if (0 == state)
        {
            for (x = 0; x < win->frame.size.x; x++)
            {
                if (i == strlen(title))
                {
                    y++;
                    state = 1 + (0 == kb_p);
                    i = 0;
                    break;
                }
                mvwaddch(win->frame.curses_win, y, x, title[i]);
                i++;
            }
            continue;
        }
        char * kb_line = get_kb_line_and_iterate(world, &kb_p);
        draw_line(win, y, kb_line, 0, 0);
        free(kb_line);
    }
    if (2 == state)
    {
        draw_line(win, y, "(none)", 0, 0);
        y++;
    }
    return y;
}



extern void draw_win_log(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    draw_text_from_bottom(win, world->log);
}



extern void draw_win_map(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    struct Map * map = world->map;
    char * cells = map->cells;
    uint16_t width_map_av  = map->size.x  - map->offset.x;
    uint16_t height_map_av = map->size.y - map->offset.y;
    uint16_t x, y, z;
    for (y = 0; y < win->frame.size.y; y++)
    {
        z = map->offset.x + (map->offset.y + y) * (map->size.x);
        for (x = 0; x < win->frame.size.x; x++)
        {
            if (y < height_map_av && x < width_map_av)
            {
                mvwaddch(win->frame.curses_win, y, x, cells[z]);
                z++;
            }
        }
    }
    draw_map_objects(world, world->map_objs, map, win);
}



extern void draw_win_info(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    char * dsc_turn      = "Turn: ";
    char * dsc_hitpoints = "\nHitpoints: ";
    char * dsc_score     = "\nScore: ";
    uint16_t maxl = strlen(dsc_turn) + strlen(dsc_hitpoints) + strlen(dsc_score)
                    + 10 + 5 + 10;       /* max strlens of numbers to be used */
    char text[maxl + 1];
    struct MapObj * player = get_player(world);
    sprintf(text, "%s%d%s%d%s%d",
            dsc_turn, world->turn,
            dsc_hitpoints, player->lifepoints,
            dsc_score, world->score);
    draw_with_linebreaks(win, text, 0);
}



extern void draw_win_inventory(struct Win * win)
{
    mvwaddstr(win->frame.curses_win, 0, 0, "(empty)");
}



extern void draw_win_available_keybindings(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    char * title = "Active window's keybindings:";
    struct KeyBinding * kb_p;
    struct WinConf * wc = get_winconf_by_win(world, world->wmeta->active);
    if     (0 == wc->view)
    {
        kb_p = wc->kb.kbs;
    }
    else if (1 == wc->view)
    {
        kb_p = world->kb_wingeom.kbs;
    }
    else if (2 == wc->view)
    {
        kb_p = world->kb_winkeys.kbs;
    }
    uint16_t offset = draw_titled_keybinding_list(world, win, 0, title, kb_p);
    draw_titled_keybinding_list(world, win, offset + 1, "Global keybindings:",
                                world->kb_global.kbs);
}



extern void draw_win_keybindings_global(struct Win * win)
{
    char * f_name = "draw_win_keybindings_global()";
    struct World * world = (struct World *) win->data;
    draw_kb_view(world, win, f_name, &world->kb_global, 0);
}



extern void draw_win_keybindings_winconf_geometry(struct Win * win)
{
    char * f_name = "draw_win_keybindings_winconf_geometry()";
    struct World * world = (struct World *) win->data;
    draw_kb_view(world, win, f_name, &world->kb_wingeom, 0);
}



extern void draw_win_keybindings_winconf_keybindings(struct Win * win)
{
    char * f_name = "draw_win_keybindings_winconf_keybindings()";
    struct World * world = (struct World *) win->data;
    draw_kb_view(world, win, f_name, &world->kb_winkeys, 0);
}



extern void draw_winconf_keybindings(struct Win * win)
{
    char * f_name = "draw_winconf_keybindings()";
    struct World * world = (struct World *) win->data;
    struct WinConf * wc = get_winconf_by_win(world, win);
    char * title = "Window's keybindings:";
    uint8_t title_space = strlen(title) / win->frame.size.x + 2;
    mvwaddstr(win->frame.curses_win, 0, 0, title);
    draw_kb_view(world, win, f_name, &wc->kb, title_space);
}



extern void draw_winconf_geometry(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    struct WinConf * wcp = get_winconf_by_win(world, win);
    char * title = "Window's geometry:\n";
    char * h_d   = "\nWidth to save: ";
    char * h_pos = " (height in cells)";
    char * h_neg = " (negative diff: cells to maximum height)";
    char * w_d   = "\n\nHeight to save: ";
    char * w_pos = " (width in cells)";
    char * w_neg = " (negative diff: cells to maximum width)";
    char * h_t = h_pos;
    char * w_t = w_pos;
    if      (1 == wcp->height_type)
    {
        h_t = h_neg;
    }
    if     (1 == wcp->width_type)
    {
        w_t = w_neg;
    }
    uint16_t maxl = strlen(title)
                    + strlen(h_t) + strlen(h_d) + 6
                    + strlen(w_t) + strlen(w_d) + 6 + 1;
    char text[maxl + 1];
    sprintf(text, "%s%s%d%s%s%d%s", title, h_d, wcp->height, h_t,
                                                w_d, wcp->width, w_t);
    draw_with_linebreaks(win, text, 0);
}
