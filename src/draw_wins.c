/* draw_wins.c */

#include "draw_wins.h"
#include <stdlib.h>      /* for free() */
#include <stdint.h>      /* for uint16_t */
#include <string.h>      /* for strlen() */
#include <ncurses.h>     /* for mvwaddch() */
#include "windows.h"     /* for structs Win, Frame, for draw_scroll_hint() */
#include "misc.h"        /* for center_offset() */
#include "keybindings.h" /* for struct KeyBinding, for get_name_to_keycode() */
#include "map_objects.h" /* for structs MapObj, Player */
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

/* Write "line" into window "win" at line "y" as far as it fits into it. */
static void draw_line(struct Win * win, uint16_t y, char * line);

/* Write "text" not starting from the top but from the bottom of "win". */
static void draw_text_from_bottom(struct Win * win, char * text);

/* Draw onto "map" in "win" the objects in the chain at "start". */
static void draw_map_objects(struct World * world, struct MapObj * start,
                             struct Map * map, struct Win * win);

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



static void draw_line(struct Win * win, uint16_t y, char * line)
{
    uint16_t x = 0;
    for (; x < win->frame.size.x && x < strlen(line); x++)
    {
        mvwaddch(win->frame.curses_win, y, x, line[x]);
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
    for (o = start; o != 0; o = o->next)
    {
        if (   o->pos.y >= map->offset.y
            && o->pos.y < map->offset.y + win->frame.size.y
            && o->pos.x >= map->offset.x
            && o->pos.x < map->offset.x + win->frame.size.x)
        {
            d = get_map_obj_def (world, o->type);
            c = d->mapchar;
            mvwaddch(win->frame.curses_win,
                     o->pos.y - map->offset.y, o->pos.x - map->offset.x, c);
        }
    }
}



static void draw_kb_view(struct World * world, struct Win * win,
                         char * f_name, struct KeyBiData * kb, uint8_t start)
{
    char * err_hint = trouble_msg(world, f_name, "draw_scroll_hint()");
    uint16_t kb_max = get_n_of_keybs(kb->kbs) - 1;
    uint16_t y, x, offset;
    offset = center_offset(kb->select, kb_max, win->frame.size.y - 1 - start);
    uint8_t keydescwidth = 9 + 1;  /* get_name_to_keycode()'s max length + \0 */
    char keydesc[keydescwidth];
    uint16_t nav_max = kb_max + start;
    uint16_t y_border = win->frame.size.y + offset - 1 - start;
    for (y = start; y <= nav_max && y < win->frame.size.y; y++)
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

        struct KeyBinding * kb_p;
        kb_p = get_keyb_of_n(kb->kbs, (y - start) + offset);
        char * keyname = get_name_to_keycode(world, kb_p->key);
        snprintf(keydesc, keydescwidth, "%-9s", keyname);
        free(keyname);
        char * cmd_dsc = get_command_longdsc(world, kb_p->name);
        uint8_t dsclen = strlen(keydesc);
        for (x = 0; x < win->frame.size.x; x++)
        {
            if (x < dsclen)
            {
                mvwaddch(win->frame.curses_win, y, x, keydesc[x] | attri);
                continue;
            }
            else if (dsclen < x && x < strlen(cmd_dsc) + strlen(keydesc) + 1)
            {
                chtype ch = cmd_dsc[x - strlen(keydesc) - 1] | attri;
                mvwaddch(win->frame.curses_win, y, x, ch);
                continue;
            }
            mvwaddch(win->frame.curses_win, y, x, ' ' | attri);
        }

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
        char * keyname = get_name_to_keycode(world, kb_p->key);
        char * cmd_dsc = get_command_longdsc(world, kb_p->name);
        char line[9 + 1 + strlen(cmd_dsc) + 1];
        sprintf(line, "%-9s %s", keyname, cmd_dsc);
        free(keyname);
        kb_p = kb_p->next;
        draw_line(win, y, line);
    }
    if (2 == state)
    {
        char * line = "(none)";
        draw_line(win, y, line);
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
    struct Player * player = world->player;
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
    draw_map_objects (world, (struct MapObj *) world->item, map, win);
    draw_map_objects (world, (struct MapObj *) world->monster, map, win);
    if (   player->pos.y >= map->offset.y
        && player->pos.y < map->offset.y + win->frame.size.y
        && player->pos.x >= map->offset.x
        && player->pos.x < map->offset.x + win->frame.size.x)
    {
        mvwaddch(win->frame.curses_win,
                 player->pos.y - map->offset.y, player->pos.x - map->offset.x,
                 '@');
    }
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
    sprintf(text, "%s%d%s%d%s%d",
            dsc_turn, world->turn,
            dsc_hitpoints, world->player->hitpoints,
            dsc_score, world->score);
    draw_with_linebreaks(win, text, 0);
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
