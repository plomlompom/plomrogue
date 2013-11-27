/* draw_wins.c */

#include "draw_wins.h"
#include <stdlib.h>      /* for free() */
#include <stdint.h>      /* for uint16_t */
#include <string.h>      /* for strlen() */
#include <ncurses.h>     /* for attri_t, chtype */
#include "windows.h"     /* for struct Win */
#include "misc.h"        /* for try_malloc() */
#include "keybindings.h" /* for struct KeyBinding, for get_name_to_keycode() */
#include "map_objects.h" /* for structs MapObj, get_map_object_def(),
                          * get_player()
                          */
#include "map.h"         /* for Map struct */
#include "main.h"        /* for world global */
#include "command_db.h"  /* for get_command_longdesc() */
#include "wincontrol.h"  /* for WinConf struct, get_winconf_by_win() */



/* Apply to the winmap of Win "w" the new sizes "new_size_y" and "new_size_x"
 * to the degree that they extend it. Re-shape the window content accordingly.
 */
static void try_resize_winmap(struct Win * w, int new_size_y, int new_size_x);

/* In Win "w", write "ch" to coordinate "y"/"x". */
static void set_ch_on_yx(struct Win * w, int y, int x, chtype ch);

/* Add "text" into window "win". Break text at right window edge. Also break at
 * newlines.
 */
static void add_text_with_linebreaks(struct Win * win, char * text);

/* Add "line" into window "w". Apply ncurses attribute "attri" to all
 * characters drawn. If "fill" is non-zero, fill the entire line until the
 * right window edge with empty characters ("attri" also applied on these).
 */
static void add_line(struct Win * w, char * line, attr_t attri, uint8_t fill);

/* Write "text" with add_text_with_linebreaks() as not starting from the top but
 * from bottom of "win". Draw only what fits in window (avoid scroll hints).
 */
static void draw_text_from_bottom(struct Win * win, char * text);

/* Draw onto "map" in "win" the objects in the chain at "start". */
static void draw_map_objects(struct MapObj * start, struct Map * map,
                             struct Win * win);

/* Return keybinding list line via "kb_pp", iterate pointer pointed to by it. */
static char * get_kb_line_and_iterate(struct KeyBinding ** kb_pp);

/* Draw from line "start" on config view for keybindings defined at "kb". */
static void draw_kb_view(struct Win * w, struct KeyBiData * kb, uint8_t start);

/* Draw into window "w" from line "start" on a "title" followed by an empty
 * line followed by a list of all keybindings starting at kb_p.
 */
static uint16_t draw_titled_keybinding_list(char * title, struct Win * w,
                                            uint16_t start,
                                            struct KeyBinding * kb_p);



static void try_resize_winmap(struct Win * w, int new_size_y, int new_size_x)
{
    char * f_name = "try_resize_winmap()";
    if (w->winmapsize.y >= new_size_y && w->winmapsize.x >= new_size_x)
    {
        return;
    }
    if      (w->winmapsize.y > new_size_y)
    {
        new_size_y = w->winmapsize.y;
    }
    else if (w->winmapsize.x > new_size_x)
    {
        new_size_x = w->winmapsize.x;
    }
    chtype * old_winmap = w->winmap;
    uint32_t new_size = sizeof(chtype) * new_size_y * new_size_x;
    w->winmap = try_malloc(new_size, f_name);
    uint16_t y, x;
    for (y = 0; y < new_size_y; y++)
    {
        for (x = 0; y < w->winmapsize.y && x < w->winmapsize.x; x++)
        {
            chtype ch = old_winmap[(y * w->winmapsize.x) + x];
            w->winmap[(y * new_size_x) + x] = ch;
        }
        for (; x < new_size_x; x++)
        {
            w->winmap[(y * new_size_x) + x] = ' ';
        }
    }
    free(old_winmap);
    w->winmapsize.y = new_size_y;
    w->winmapsize.x = new_size_x;
}



static void set_ch_on_yx(struct Win * w, int y, int x, chtype ch)
{
    w->winmap[(y * w->winmapsize.x) + x] = ch;
}



static void add_text_with_linebreaks(struct Win * win, char * text)
{
    uint16_t x, y;
    int16_t z = -1;
    for (y = win->winmapsize.y; ; y++)
    {
        try_resize_winmap(win, y + 1, win->framesize.x);
        for (x = 0; x < win->framesize.x; x++)
        {
            z++;
            if ('\n' == text[z])
            {
                break;
            }
            else
            {
                set_ch_on_yx(win, y, x, text[z]);
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



static void add_line(struct Win * w, char * line, attr_t attri, uint8_t fill)
{
    uint16_t y = w->winmapsize.y;
    uint16_t len_line = strlen(line);
    if (0 != fill
        && w->winmapsize.x < w->framesize.x && w->framesize.x > len_line)
    {
        try_resize_winmap(w, y + 1, w->framesize.x);
    }
    else
    {
        try_resize_winmap(w, y + 1, strlen(line));
    }
    uint16_t x = 0;
    for (; x < len_line; x++)
    {
        set_ch_on_yx(w, y, x, line[x] | attri);
    }
    if (0 != fill)
    {
        for (; x < w->framesize.x; x++)
        {
            set_ch_on_yx(w, y, x, ' ' | attri);
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
    uint16_t x, y;
    int16_t z = -1;
    for (y = 0; 0 == toggle; y++)
    {
        for (x = 0; x < win->framesize.x; x++)
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
    if (y < win->framesize.y)
    {
        start_y = win->framesize.y - y;
    }
    else if (y > win->framesize.y)
    {
        uint16_t offset = y - win->framesize.y;
        for (y = 0; y < offset; y++)
        {
            for (x = 0; x < win->framesize.x; x++)
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

    try_resize_winmap(win, start_y, 1);
    add_text_with_linebreaks(win, text);
}



static void draw_map_objects(struct MapObj * start, struct Map * map,
                             struct Win * win)
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
                d = get_map_object_def(o->type);
                c = d->char_on_map;
                set_ch_on_yx(win, o->pos.y, o->pos.x, c);
            }
        }
    }
}



static char * get_kb_line_and_iterate(struct KeyBinding ** kb_pp)
{
    char * f_name = "get_kb_line_and_iterate()";
    struct KeyBinding * kb_p = * kb_pp;
    char * keyname = get_name_to_keycode(kb_p->key);
    char * cmd_dsc = get_command_longdsc(kb_p->name);
    uint16_t size = 9 + 1 + strlen(cmd_dsc) + 1;
    char * line = try_malloc(size, f_name);
    sprintf(line, "%-9s %s", keyname, cmd_dsc);
    free(keyname);
    * kb_pp = kb_p->next;
    return line;
}



static void draw_kb_view(struct Win * w, struct KeyBiData * kb, uint8_t start)
{
    if (0 == kb->kbs)
    {
        add_line(w, "(none)", 0, 0);
        return;
    }
    struct KeyBinding * kb_p = kb->kbs;
    uint16_t y;
    for (y = start; 0 != kb_p; y++)
    {
        attr_t attri = 0;
        if (y - start == kb->select)
        {
            attri = A_REVERSE;
            if (1 == kb->edit)
            {
                attri = attri | A_BLINK;
            }
        }
        char * kb_line = get_kb_line_and_iterate(&kb_p);
        add_line(w, kb_line, attri, 1);
        free(kb_line);
    }
}



static uint16_t draw_titled_keybinding_list(char * title, struct Win * w,
                                            uint16_t start,
                                            struct KeyBinding * kb_p)
{
    uint16_t y;
    uint8_t state = 0;
    for (y = start; (0 == state || 0 != kb_p); y++)
    {
        if (0 == state)
        {
            add_line(w, title, 0, 0);
            y++;
            add_line(w, " ", 0, 0);
            state = 1 + (0 == kb_p);
            continue;
        }
        char * kb_line = get_kb_line_and_iterate(&kb_p);
        add_line(w, kb_line, 0, 0);
        free(kb_line);
    }
    if (2 == state)
    {
        char * none = "(none)";
        add_line(w, none, 0, 0);
        y++;
    }
    return y;
}



extern void draw_win_log(struct Win * win)
{
    draw_text_from_bottom(win, world.log);
}



extern void draw_win_map(struct Win * win)
{
    struct Map * map = world.map;
    char * cells = map->cells;
    try_resize_winmap(win, map->size.y, map->size.x);
    uint16_t z = 0;
    uint16_t x, y;
    for (y = 0; y < map->size.y; y++)
    {
        for (x = 0; x < map->size.x; x++)
        {
            set_ch_on_yx(win, y, x, cells[z]);
            z++;
        }
    }
    draw_map_objects(world.map_objs, map, win);
}



extern void draw_win_info(struct Win * win)
{
    char * dsc_turn      = "Turn: ";
    char * dsc_hitpoints = "\nHitpoints: ";
    char * dsc_score     = "\nScore: ";
    uint16_t maxl = strlen(dsc_turn) + strlen(dsc_hitpoints) + strlen(dsc_score)
                    + 10 + 5 + 10;       /* max strlens of numbers to be used */
    char text[maxl + 1];
    struct MapObj * player = get_player();
    sprintf(text, "%s%d%s%d%s%d",
            dsc_turn, world.turn,
            dsc_hitpoints, player->lifepoints,
            dsc_score, world.score);
    add_text_with_linebreaks(win, text);
}



extern void draw_win_inventory(struct Win * win)
{
    struct MapObj * player = get_player();
    if (NULL == player->owns)
    {
        add_line(win, "(none)", 0, 0);
        return;
    }
    win->center.y = world.inventory_sel;
    struct MapObj * owned = player->owns;
    uint8_t y;
    for (y = 0; NULL != owned; y++)
    {
        attr_t attri = 0;
        if (y == world.inventory_sel)
        {
            attri = A_REVERSE;
        }
        struct MapObjDef * mod = get_map_object_def(owned->type);
        add_line(win, mod->name, attri, 0);
        owned = owned->next;
    }
}



extern void draw_win_available_keybindings(struct Win * win)
{
    char * title = "Active window's keybindings:";
    struct KeyBinding * kb_p;
    struct WinConf * wc = get_winconf_by_win(world.wmeta->active);
    if     (0 == wc->view)
    {
        kb_p = wc->kb.kbs;
    }
    else if (1 == wc->view)
    {
        kb_p = world.kb_wingeom.kbs;
    }
    else if (2 == wc->view)
    {
        kb_p = world.kb_winkeys.kbs;
    }
    uint16_t offset = draw_titled_keybinding_list(title, win, 0, kb_p);
    add_line(win, " ", 0, 0);
    struct KeyBinding * kbs_glo = world.kb_global.kbs;
    draw_titled_keybinding_list("Global keybindings", win, offset + 1, kbs_glo);
}



extern void draw_win_keybindings_global(struct Win * win)
{
    win->center.y = world.kb_global.select;
    draw_kb_view(win, &world.kb_global, 0);
}



extern void draw_win_keybindings_winconf_geometry(struct Win * win)
{
    win->center.y = world.kb_wingeom.select;
    draw_kb_view(win, &world.kb_wingeom, 0);
}



extern void draw_win_keybindings_winconf_keybindings(struct Win * win)
{
    win->center.y = world.kb_winkeys.select;
    draw_kb_view(win, &world.kb_winkeys, 0);
}



extern void draw_winconf_keybindings(struct Win * win)
{
    struct WinConf * wc = get_winconf_by_win(win);
    char * title = "Window's keybindings:";
    add_line(win, title, 0, 0);
    add_line(win, " ", 0, 0);
    draw_kb_view(win, &wc->kb, 2);
    win->center.y = wc->kb.select + 2;
}



extern void draw_winconf_geometry(struct Win * win)
{
    struct WinConf * wcp = get_winconf_by_win(/*&world, */win);
    char * title = "Window's geometry:\n";
    char * h_d   = "\nHeight to save: ";
    char * h_pos = " (width in cells)";
    char * h_neg = " (negative diff: cells to screen width)";
    char * w_d   = "\n\nWidth to save: ";
    char * w_pos = " (height in cells)";
    char * w_neg = " (negative diff: cells to screen height)";
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
    add_text_with_linebreaks(win, text);
}
