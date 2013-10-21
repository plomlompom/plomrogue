/* draw_wins.c */

#include "draw_wins.h"
#include <stdlib.h>      /* for free() */
#include <stdint.h>      /* for uint16_t */
#include <string.h>      /* for strlen() */
#include <ncurses.h>     /* for attri_t, chtype, mvwaddch(), wresize() */
#include "windows.h"     /* for struct Win */
#include "misc.h"        /* for try_malloc() */
#include "keybindings.h" /* for struct KeyBinding, for get_name_to_keycode() */
#include "map_objects.h" /* for structs MapObj, get_map_object_def(),
                          * get_player()
                          */
#include "map.h"         /* for Map struct */
#include "main.h"        /* for World struct */
#include "command_db.h"  /* for get_command_longdesc() */
#include "wincontrol.h"  /* for WinConf struct, get_winconf_by_win() */



/* Widen the ncurses window below "win" to "minx" if it is below that. */
static void rewiden_if_less_than(struct Win * win, uint16_t minx);

/* Write "text" into window "win". Start on row "start_y". Break text at
 * right window edge. Also break at newlines.
 */
static void draw_with_linebreaks(struct Win * win, char * text,
                                 uint16_t start_y);

/* Write "line" into window "w" at line "y"; apply ncurses attribute
 * "attri" to all characters drawn; if "fill" is non-zero, fill the
 * entire line until the right window edge with empty characters
 * ("attri" also applied on these).
 */
static void draw_line(struct Win * w, uint16_t y, char * line, attr_t attri,
                      uint8_t fill);

/* Write "text" with draw_with_linebreaks() as not starting from the top
 * but from the bottom of "win".
 */
static void draw_text_from_bottom(struct Win * win, char * text);

/* Draw onto "map" in "win" the objects in the chain at "start". */
static void draw_map_objects(struct World * world, struct MapObj * start,
                             struct Map * map, struct Win * win);

/* Return keybinding list line via "kb_pp", iterate pointer pointed to by it. */
static char * get_kb_line_and_iterate(struct World * world,
                                      struct KeyBinding ** kb_pp);

/* Draw from line "start" on config view for keybindings defined at "kb". */
static void draw_kb_view(struct World * world, struct Win * w,
                         struct KeyBiData * kb, uint8_t start);

/* Draw into window "w" from line "start" on a "title" followed by an empty
 * line followed by a list of all keybindings starting at kb_p.
 */
static uint16_t draw_titled_keybinding_list(struct World * world, char * title,
                                            struct Win * w, uint16_t start,
                                            struct KeyBinding * kb_p);



static void rewiden_if_less_than(struct Win * win, uint16_t minx)
{
    if (minx > getmaxx(win->frame.curses_win))
    {
        wresize(win->frame.curses_win, getmaxy(win->frame.curses_win), minx);
    }
}



static void draw_with_linebreaks(struct Win * win, char * text,
                                 uint16_t start_y)
{
    uint16_t x, y;
    int16_t z = -1;
    rewiden_if_less_than(win, win->frame.size.x);
    for (y = start_y; ; y++)
    {
        wresize(win->frame.curses_win, y + 1, getmaxx(win->frame.curses_win));
        for (x = 0; x < win->frame.size.x; x++)
        {
            z++;
            if ('\n' == text[z])
            {
                break;
            }
            else
            {
                mvwaddch(win->frame.curses_win, y, x, text[z]);
            }
            if ('\n' == text[z+1])
            {
                z++;
                break;
            }
            else if (0 == text[z+1])
            {
                return;
            }
        }
    }
}



static void draw_line(struct Win * w, uint16_t y, char * line, attr_t attri,
                      uint8_t fill)
{
    uint16_t x = 0;
    for (; x < strlen(line); x++)
    {
        rewiden_if_less_than(w, x + 1);
        mvwaddch(w->frame.curses_win, y, x, line[x] | attri);
    }
    if (0 != fill)
    {
        for (; x < w->frame.size.x; x++)
        {
            rewiden_if_less_than(w, x + 1);
            mvwaddch(w->frame.curses_win, y, x, ' ' | attri);
        }
    }
}



static void draw_text_from_bottom(struct Win * win, char * text)
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
            if ((   (0 == i && 0 == o->lifepoints)         /* Draw in-animate */
                 || (1 == i && 0 < o->lifepoints)))        /* objects first.  */
            {
                d = get_map_object_def(world, o->type);
                c = d->char_on_map;
                mvwaddch(win->frame.curses_win, o->pos.y, o->pos.x, c);
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



static void draw_kb_view(struct World * world, struct Win * w,
                         struct KeyBiData * kb, uint8_t start)
{
    if (0 == kb->kbs)
    {
        wresize(w->frame.curses_win, start + 1, getmaxx(w->frame.curses_win));
        draw_line(w, start, "(none)", 0, 0);
        return;
    }
    struct KeyBinding * kb_p = kb->kbs;
    uint16_t y;
    for (y = start; 0 != kb_p; y++)
    {
        wresize(w->frame.curses_win, y + 1, getmaxx(w->frame.curses_win));
        attr_t attri = 0;
        if (y - start == kb->select)
        {
            attri = A_REVERSE;
            if (1 == kb->edit)
            {
                attri = attri | A_BLINK;
            }
        }
        char * kb_line = get_kb_line_and_iterate(world, &kb_p);
        draw_line(w, y, kb_line, attri, 1);
        free(kb_line);
    }
}



static uint16_t draw_titled_keybinding_list(struct World * world, char * title,
                                            struct Win * w, uint16_t start,
                                            struct KeyBinding * kb_p)
{
    uint16_t x, y;
    uint16_t i = 0;
    uint8_t state = 0;
    for (y = start; (0 == state || 0 != kb_p); y++)
    {
        wresize(w->frame.curses_win, y + 1, getmaxx(w->frame.curses_win));
        if (0 == state)
        {
            for (x = 0; ; x++)
            {
                if (i == strlen(title))
                {
                    y++;
                    state = 1 + (0 == kb_p);
                    i = 0;
                    break;
                }
                rewiden_if_less_than(w, x + 1);
                mvwaddch(w->frame.curses_win, y, x, title[i]);
                i++;
            }
            continue;
        }
        char * kb_line = get_kb_line_and_iterate(world, &kb_p);
        if (strlen(kb_line) > getmaxx(w->frame.curses_win))
        {
            wresize(w->frame.curses_win, y + 1, strlen(kb_line));
        }
        draw_line(w, y, kb_line, 0, 0);
        free(kb_line);
    }
    if (2 == state)
    {
        char * none = "(none)";
        if (strlen(none) > getmaxx(w->frame.curses_win))
        {
            wresize(w->frame.curses_win, y + 1, strlen(none));
        }
        else
        {
            wresize(w->frame.curses_win, y + 1, getmaxx(w->frame.curses_win));
        }
        draw_line(w, y, none, 0, 0);
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
    wresize(win->frame.curses_win, map->size.y, map->size.x);
    uint16_t x, y, z;
    for (y = 0; y < map->size.y; y++)
    {
        for (x = 0; x < map->size.x; x++)
        {
            mvwaddch(win->frame.curses_win, y, x, cells[z]);
            z++;
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
    struct World * world = (struct World *) win->data;
    struct MapObj * player = get_player(world);
    if (NULL == player->owns)
    {
        draw_line(win, 0, "(none)", 0, 0);
        return;
    }
    win->center.y = world->inventory_select;
    struct MapObj * owned = player->owns;
    uint8_t y;
    for (y = 0; NULL != owned; y++)
    {
        wresize(win->frame.curses_win, y + 1, getmaxx(win->frame.curses_win));
        attr_t attri = 0;
        if (y == world->inventory_select)
        {
            attri = A_REVERSE;
        }
        struct MapObjDef * mod = get_map_object_def(world, owned->type);
        draw_line(win, y, mod->name, attri, 0);
        owned = owned->next;
    }
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
    uint16_t offset = draw_titled_keybinding_list(world, title, win, 0, kb_p);
    draw_titled_keybinding_list(world, "Global keybindings", win, offset + 1,
                                world->kb_global.kbs);
}



extern void draw_win_keybindings_global(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    win->center.y = world->kb_global.select;
    draw_kb_view(world, win, &world->kb_global, 0);
}



extern void draw_win_keybindings_winconf_geometry(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    win->center.y = world->kb_wingeom.select;
    draw_kb_view(world, win, &world->kb_wingeom, 0);
}



extern void draw_win_keybindings_winconf_keybindings(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    win->center.y = world->kb_winkeys.select;
    draw_kb_view(world, win, &world->kb_winkeys, 0);
}



extern void draw_winconf_keybindings(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    struct WinConf * wc = get_winconf_by_win(world, win);
    char * title = "Window's keybindings:";
    draw_line(win, 0, title, 0, 0);
    draw_kb_view(world, win, &wc->kb, 2);
    win->center.y = wc->kb.select + 2;
}



extern void draw_winconf_geometry(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    struct WinConf * wcp = get_winconf_by_win(world, win);
    char * title = "Window's geometry:\n";
    char * h_d   = "\nHeight to save: ";
    char * h_pos = " (width in cells)";
    char * h_neg = " (negative diff: cells to maximum width)";
    char * w_d   = "\n\nWidth to save: ";
    char * w_pos = " (height in cells)";
    char * w_neg = " (negative diff: cells to maximum height)";
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
                                           w_d, wcp->width,  w_t);
    draw_with_linebreaks(win, text, 0);
}
