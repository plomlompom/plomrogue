/* src/client/draw_wins.c */

#include "draw_wins.h"
#include <ncurses.h> /* attri_t, chtype */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, int16_t */
#include <stdio.h> /* for sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), strtok() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "keybindings.h" /* struct KeyBinding, get_keyname_to_keycode() */
#include "windows.h" /* struct Win, get_win_by_id() */
#include "world.h" /* global world */



/* Apply to the winmap of Win "w" the new sizes "new_size_y" and "new_size_x"
 * to the degree that they extend it. Re-shape the window content accordingly.
 */
static void try_resize_winmap(struct Win * w, int new_size_y, int new_size_x);

/* In Win "w", write "ch" to coordinate "y"/"x". */
static void set_ch_on_yx(struct Win * w, int y, int x, chtype ch);

/* Add "text" into window "win". Break text at right window edge. Also break at
 * newlines. If "text" ends in a newline, ignore it.
 */
static void add_text_with_linebreaks(struct Win * win, char * text);

/* Add "line" into window "w". Apply ncurses attribute "attri" to all
 * characters drawn. If "attri" is non-zero, fill the entire line until the
 * right window edge with empty characters, so "attri" applies on those too.
 */
static void add_line(struct Win * w, char * line, attr_t attri);

/* Write "text" with add_text_with_linebreaks() as not starting from the top but
 * from bottom of "win". Draw only what fits in window (avoid scroll hints).
 */
static void draw_text_from_bottom(struct Win * win, char * text);

/* Return a properly formatted keybinding list line for "kb". */
static char * get_kb_line(struct KeyBinding * kb);

/* Draw from line "start" on config view for keybindings defined at "kb". */
static void draw_keybinding_config(struct Win * w, struct KeyBindingDB * kb,
                                   uint8_t start);

/* Draw into window "w" from line "start" on a "title" followed by an empty
 * line followed by a list of all keybindings starting in "kbdb".
 */
static uint16_t draw_titled_keybinding_list(char * title, struct Win * w,
                                            uint16_t start,
                                            struct KeyBindingDB * kbdb);



static void try_resize_winmap(struct Win * w, int new_size_y, int new_size_x)
{
    char * f_name = "try_resize_winmap()";
    if (w->winmap_size.y >= new_size_y && w->winmap_size.x >= new_size_x)
    {
        return;
    }
    if      (w->winmap_size.y > new_size_y)
    {
        new_size_y = w->winmap_size.y;
    }
    else if (w->winmap_size.x > new_size_x)
    {
        new_size_x = w->winmap_size.x;
    }
    chtype * old_winmap = w->winmap;
    uint32_t new_size = sizeof(chtype) * new_size_y * new_size_x;
    w->winmap = try_malloc(new_size, f_name);
    uint16_t y, x;
    for (y = 0; y < new_size_y; y++)
    {
        for (x = 0; y < w->winmap_size.y && x < w->winmap_size.x; x++)
        {
            chtype ch = old_winmap[(y * w->winmap_size.x) + x];
            w->winmap[(y * new_size_x) + x] = ch;
        }
        for (; x < new_size_x; x++)
        {
            w->winmap[(y * new_size_x) + x] = ' ';
        }
    }
    free(old_winmap);
    w->winmap_size.y = new_size_y;
    w->winmap_size.x = new_size_x;
}



static void set_ch_on_yx(struct Win * w, int y, int x, chtype ch)
{
    w->winmap[(y * w->winmap_size.x) + x] = ch;
}



static void add_text_with_linebreaks(struct Win * win, char * text)
{
    uint16_t x, y;
    int16_t z = -1;
    for (y = win->winmap_size.y; ; y++)
    {
        try_resize_winmap(win, y + 1, win->frame_size.x);
        for (x = 0; x < win->frame_size.x; x++)
        {
            z++;
            if      ('\n' == text[z])
            {
                break;
            }
            else if ('\0' == text[z])
            {
                return;
            }
            else
            {
                set_ch_on_yx(win, y, x, text[z]);
            }
            if      ('\n' == text[z+1])
            {
                z++;
                break;
            }
            else if ('\0' == text[z+1])
            {
                return;
            }
        }
    }
}



static void add_line(struct Win * w, char * line, attr_t attri)
{
    uint16_t y = w->winmap_size.y;
    uint16_t len_line = strlen(line);
    if (attri
        && w->winmap_size.x < w->frame_size.x && w->frame_size.x > len_line)
    {
        try_resize_winmap(w, y + 1, w->frame_size.x);
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
    if (attri)
    {
        for (; x < w->frame_size.x; x++)
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
        for (x = 0; x < win->frame_size.x; x++)
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
    if (y < win->frame_size.y)
    {
        start_y = win->frame_size.y - y;
    }
    else if (y > win->frame_size.y)
    {
        uint16_t offset = y - win->frame_size.y;
        for (y = 0; y < offset; y++)
        {
            for (x = 0; x < win->frame_size.x; x++)
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



static char * get_kb_line(struct KeyBinding * kb)
{
    char * f_name = "get_kb_line()";
    char * keyname = get_keyname_to_keycode(kb->keycode);
    uint16_t size = 9 + 1 + strlen(kb->command->dsc_long) + 1;
    char * kb_line = try_malloc(size, f_name);
    sprintf(kb_line, "%-9s %s", keyname, kb->command->dsc_long);
    free(keyname);
    return kb_line;
}



static void draw_keybinding_config(struct Win * w, struct KeyBindingDB * kbdb,
                                   uint8_t start)
{
    if (0 == kbdb->n_of_kbs)
    {
        add_line(w, "(none)", 0);
        return;
    }
    uint16_t y, kb_n;
    for (y = start, kb_n = 0; kb_n < kbdb->n_of_kbs; y++, kb_n++)
    {
        attr_t attri = 0;
        if (y - start == kbdb->select)
        {
            attri = A_REVERSE;
            if (1 == kbdb->edit)
            {
                attri = attri | A_BLINK;
            }
        }
        char * kb_line = get_kb_line(&kbdb->kbs[kb_n]);
        add_line(w, kb_line, attri);
        free(kb_line);
    }
}



static uint16_t draw_titled_keybinding_list(char * title, struct Win * w,
                                            uint16_t start,
                                            struct KeyBindingDB * kbdb)
{
    uint16_t y;
    uint8_t state = 0;
    uint16_t kb_n = 0;
    for (y = start; (0 == state || kb_n < kbdb->n_of_kbs); y++, kb_n++)
    {
        if (0 == state)
        {
            add_line(w, title, 0);
            y++;
            add_line(w, " ", 0);
            state = 1 + (0 == kbdb->n_of_kbs);
            continue;
        }
        char * kb_line = get_kb_line(&kbdb->kbs[kb_n]);
        add_line(w, kb_line, 0);
        free(kb_line);
    }
    if (2 == state)
    {
        char * none = "(none)";
        add_line(w, none, 0);
        y++;
    }
    return y;
}



extern void draw_win_log(struct Win * win)
{
    char * log = "";
    if (world.log)
    {
        log = world.log;
    }
    draw_text_from_bottom(win, log);
}



extern void draw_win_map(struct Win * win)
{
    try_resize_winmap(win, world.map.size.y, world.map.size.x);
    uint16_t z = 0;
    uint16_t x, y;
    for (y = 0; y < world.map.size.y; y++)
    {
        for (x = 0; x < world.map.size.x; x++)
        {
            set_ch_on_yx(win, y, x, world.map.cells[z]);
            z++;
        }
    }
}



extern void draw_win_info(struct Win * win)
{
    char * dsc_turn      = "Turn: ";
    char * dsc_hitpoints = "\nHitpoints: ";
    uint16_t maxl = strlen(dsc_turn) + 5 + strlen(dsc_hitpoints) + 3;
    char text[maxl + 1];
    sprintf(text, "%s%d%s%d",
            dsc_turn, world.turn, dsc_hitpoints, world.player_lifepoints);
    add_text_with_linebreaks(win, text);
}



extern void draw_win_inventory(struct Win * win)
{
    win->center.y = world.player_inventory_select;
    char inventory_copy[strlen(world.player_inventory) + 1];
    sprintf(inventory_copy, "%s", world.player_inventory);
    char * strtok_target = inventory_copy;
    uint8_t i = 0;
    while (1)
    {
        char * object = strtok(strtok_target, "\n");
        strtok_target = NULL;
        if (NULL == object)
        {
            return;
        }
        attr_t attri = 0;
        if (i == world.player_inventory_select)
        {
            attri = A_REVERSE;
        }
        add_line(win, object, attri);
        i++;
    }
}



extern void draw_win_available_keybindings(struct Win * win)
{
    char * title = "Active window's keybindings:";
    struct KeyBindingDB * kbdb;
    struct Win * w = get_win_by_id(world.winDB.active);
    if     (0 == w->view)
    {
        kbdb = &w->kb;
    }
    else if (1 == w->view)
    {
        kbdb = &world.kb_wingeom;
    }
    else if (2 == w->view)
    {
        kbdb = &world.kb_winkeys;
    }
    uint16_t offset = draw_titled_keybinding_list(title, win, 0, kbdb);
    add_line(win, " ", 0);
    draw_titled_keybinding_list("Global keybindings", win, offset + 1,
                                &world.kb_global);
}



extern void draw_win_keybindings_global(struct Win * win)
{
    win->center.y = world.kb_global.select;
    draw_keybinding_config(win, &world.kb_global, 0);
}



extern void draw_win_keybindings_winconf_geometry(struct Win * win)
{
    win->center.y = world.kb_wingeom.select;
    draw_keybinding_config(win, &world.kb_wingeom, 0);
}



extern void draw_win_keybindings_winconf_keybindings(struct Win * win)
{
    win->center.y = world.kb_winkeys.select;
    draw_keybinding_config(win, &world.kb_winkeys, 0);
}



extern void draw_winconf_keybindings(struct Win * win)
{
    char * title = "Window's keybindings:";
    add_line(win, title, 0);
    add_line(win, " ", 0);
    draw_keybinding_config(win, &win->kb, 2);
    win->center.y = win->kb.select + 2;
}



extern void draw_winconf_geometry(struct Win * win)
{
    char * title = "Window's geometry:\n";
    char * h_d   = "\nHeight to save: ";
    char * h_pos = " (width in cells)";
    char * h_neg = " (negative diff: cells to screen width)";
    char * w_d   = "\n\nWidth to save: ";
    char * w_pos = " (height in cells)";
    char * w_neg = " (negative diff: cells to screen height)";
    char * h_t = h_pos;
    char * w_t = w_pos;
    if (1 == win->target_height_type)
    {
        h_t = h_neg;
    }
    if (1 == win->target_width_type)
    {
        w_t = w_neg;
    }
    uint16_t maxl = strlen(title)
                    + strlen(h_t) + strlen(h_d) + 6      /* 6 = n of chars to */
                    + strlen(w_t) + strlen(w_d) + 6 + 1; /* write max int16_t */
    char text[maxl + 1];
    sprintf(text, "%s%s%d%s%s%d%s", title, h_d, win->target_height, h_t,
                                           w_d, win->target_width,  w_t);
    add_text_with_linebreaks(win, text);
}
