/* src/client/draw_wins.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "draw_wins.h"
#include <ncurses.h> /* attr_t, chtype, init_pair(), A_REVERSE, COLOR_PAIR() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* memset(), strchr(), strdup/(), strlen() */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "keybindings.h" /* struct KeyBindingDB, get_keyname_to_keycode() */
#include "windows.h" /* yx_uint16, Win, get_win_by_id() */
#include "world.h" /* global world */



/* Apply to the winmap of Win "w" the new sizes "new_size_y" and "new_size_x"
 * to the degree that they extend it. Re-shape the window content accordingly.
 */
static void try_resize_winmap(struct Win * w, int new_size_y, int new_size_x);

/* In Win "w", write "ch" to coordinate "y"/"x". */
static void set_ch_on_yx(struct Win * w, int y, int x, chtype ch);

/* Add "line" into window "w" with "attri" set on all chars. add_line() selects
 * one of the other three functions based on the "win"->linebreak mode.
 *
 * "wide" writes lines to full width without in-between breaks. "long" breaks
 * lines on the right window border. "compact" replaces newlines with printable
 * separator characters. "offset" and "last" are only relevant to the "compact"
 * mode. "offset" reports the horizontal offset at which the line is drawn from
 * the lowest existing winmap line on; it is updated to the next offset after
 * the new line, to be used by the next add_line_compact() call. "last" marks
 * the line to add as the last in the window, so no further separator is added.
 */
static void add_line(struct Win * win, char * line, attr_t attri,
                     uint16_t * offset, uint8_t last);
static void add_line_wide(struct Win * win, char * line, attr_t attri);
static void add_line_long(struct Win * win, char * line, attr_t attri);
static void add_line_compact(struct Win * win, char * line, attr_t attri,
                             uint16_t * offset, uint8_t last);

/* Add linebreaks-containing "text" to "win", formatted as per "win"->linebreak.
 *
 * draw_text_from_bottom() prepends empty lines to "text" to push its last line
 * to the low frame edge when the frame is of greater height than the winmap
 * would be otherwise; else, set "win"->center.y to the winmap's lowest line.
 */
static void add_text_with_linebreaks(struct Win * win, char * text);
static void draw_text_from_bottom(struct Win * win, char * text);

/* Return a properly formatted keybinding list line for "kb". */
static char * get_kb_line(struct KeyBinding * kb);

/* Draw from line "start" on config view for keybindings defined at "kb". */
static void draw_keybinding_config(struct Win * win, struct KeyBindingDB * kbdb,
                                   uint16_t offset);



static void try_resize_winmap(struct Win * win, int new_size_y, int new_size_x)
{
    if (win->winmap_size.y >= new_size_y && win->winmap_size.x >= new_size_x)
    {
        return;
    }
    if      (win->winmap_size.y > new_size_y)
    {
        new_size_y = win->winmap_size.y;
    }
    else if (win->winmap_size.x > new_size_x)
    {
        new_size_x = win->winmap_size.x;
    }
    chtype * old_winmap = win->winmap;
    uint32_t new_size = sizeof(chtype) * new_size_y * new_size_x;
    win->winmap = try_malloc(new_size, __func__);
    uint16_t y, x;
    for (y = 0; y < new_size_y; y++)
    {
        for (x = 0; y < win->winmap_size.y && x < win->winmap_size.x; x++)
        {
            chtype ch = old_winmap[(y * win->winmap_size.x) + x];
            win->winmap[(y * new_size_x) + x] = ch;
        }
        for (; x < new_size_x; x++)
        {
            win->winmap[(y * new_size_x) + x] = ' ';
        }
    }
    free(old_winmap);
    win->winmap_size.y = new_size_y;
    win->winmap_size.x = new_size_x;
}



static void set_ch_on_yx(struct Win * win, int y, int x, chtype ch)
{
    win->winmap[(y * win->winmap_size.x) + x] = ch;
}



static void add_line(struct Win * win, char * line, attr_t attri,
                     uint16_t * offset, uint8_t last)
{
    exit_err(strlen(line) > UINT16_MAX, "Line in add_line() too big.");
    if      (0 == win->linebreak)
    {
        add_line_long(win, line, attri);
    }
    else if (1 == win->linebreak)
    {
        add_line_wide(win, line, attri);
    }
    else if (2 == win->linebreak)
    {
        add_line_compact(win, line, attri, offset, last);
    }
}



static void add_line_wide(struct Win * win, char * line, attr_t attri)
{
    uint16_t y_start = win->winmap_size.y;
    uint16_t len_line = strlen(line);
    try_resize_winmap(win, y_start + 1, win->frame_size.x);
    try_resize_winmap(win, y_start + 1, len_line);
    uint16_t x;
    for (x = 0; x < len_line; x++)
    {
        set_ch_on_yx(win, y_start, x, line[x] | attri);
    }
    if (attri)
    {
        for (; x < win->frame_size.x; x++)
        {
            set_ch_on_yx(win, y_start, x, ' ' | attri);
        }
    }
}



static void add_line_long(struct Win * win, char * line, attr_t attri)
{
    uint16_t y_start = win->winmap_size.y;
    uint16_t len_line = strlen(line);
    try_resize_winmap(win, y_start + 1, win->frame_size.x);
    uint16_t x, y, z;
    for (z = 0, y = y_start; z < len_line; y++)
    {
        for (x = 0; x < win->winmap_size.x && z < len_line; x++, z++)
        {
            set_ch_on_yx(win, y, x, line[z] | attri);
        }
        if (z < len_line)
        {
            try_resize_winmap(win, y + 1 + 1, win->winmap_size.x);
        }
    }
}



static void add_line_compact(struct Win * win, char * line, attr_t attri,
                             uint16_t * offset, uint8_t last)
{
    uint16_t y_start = win->winmap_size.y - (win->winmap_size.y > 0);
    try_resize_winmap(win, y_start + 1, win->frame_size.x);
    uint16_t len_line = strlen(line);
    char * separator = last ? "" : " / ";
    uint32_t len_line_new = len_line + strlen(separator);
    char * line_new = try_malloc(len_line_new + 1, __func__);
    int test = sprintf(line_new, "%s%s", line, separator);
    exit_trouble(test < 0, __func__, "sprintf");
    uint16_t x = 0;
    uint16_t y;
    uint32_t z;
    for (z = 0, y = y_start; z < len_line_new; y++)
    {
        for (x = * offset; x < win->winmap_size.x && z < len_line_new; x++, z++)
        {
            set_ch_on_yx(win, y, x, line_new[z] | attri);
            if (z + 1 == (uint32_t) len_line)
            {
               attri = 0;
            }
        }
        * offset = 0;
        if (z < len_line_new)
        {
            try_resize_winmap(win, y + 1 + 1, win->winmap_size.x);
        }
    }
    free(line_new);
    *offset = x;
}



static void add_text_with_linebreaks(struct Win * win, char * text)
{
    char * limit;
    uint16_t offset = 0;
    char * text_copy = strdup(text);
    char * start = text_copy;
    while (NULL != (limit = strchr(start, '\n')))
    {
        (* limit) = '\0';
        add_line(win, start, 0, &offset, 0);
        start = limit + 1;
    }
    add_line(win, start, 0, &offset, 1);
    free(text_copy);
}



static void draw_text_from_bottom(struct Win * win, char * text)
{
    add_text_with_linebreaks(win, text);
    if (win->winmap_size.y > win->frame_size.y)
    {
        win->center.y = win->winmap_size.y - 1;
    }
    else if (win->winmap_size.y < win->frame_size.y)
    {
        uint16_t new_y_start = win->frame_size.y - win->winmap_size.y;
        memset(&win->winmap_size, 0, sizeof(struct yx_uint16));
        free(win->winmap);
        win->winmap = NULL;
        do
        {
            add_line_wide(win, " ", 0);
        }
        while (new_y_start > win->winmap_size.y);
        if (2 == win->linebreak) /* add_text_with_linebreaks() will start not */
        {                        /* not after, but within the last line then. */
            add_line_wide(win, " ", 0);
        }
        add_text_with_linebreaks(win, text);
    }
}



static char * get_kb_line(struct KeyBinding * kb)
{
    char * keyname = get_keyname_to_keycode(kb->keycode);
    uint16_t size = strlen(keyname) + 3 + strlen(kb->command->dsc_long) + 1;
    char * kb_line = try_malloc(size, __func__);
    int test = sprintf(kb_line, "%s - %s", keyname, kb->command->dsc_long);
    exit_trouble(test < 0, __func__, "sprintf");
    free(keyname);
    return kb_line;
}



static void draw_keybinding_config(struct Win * win, struct KeyBindingDB * kbdb,
                                   uint16_t offset)
{
    if (0 == kbdb->n_of_kbs)
    {
        add_line(win, "(none)", 0, &offset, 0);
        return;
    }
    uint8_t kb_n;
    for (kb_n = 0; kb_n < kbdb->n_of_kbs; kb_n++)
    {
        attr_t attri = 0;
        if (kb_n == kbdb->select)
        {
            attri = A_REVERSE;
            if (1 == kbdb->edit)
            {
                attri = attri | A_BLINK;
            }
            win->center.y = win->winmap_size.y;
        }
        char * kb_line = get_kb_line(&kbdb->kbs[kb_n]);
        add_line(win, kb_line, attri, &offset, (kbdb->n_of_kbs == kb_n + 1));
        free(kb_line);
    }
}



extern void draw_win_log(struct Win * win)
{
    if (!world.log)
    {
        return;
    }
    draw_text_from_bottom(win, world.log);
}



extern void draw_win_map(struct Win * win)
{
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    attr_t attr_fov = 0;
    attr_t attr_mem = COLOR_PAIR(2);
    attr_t attr_sha = COLOR_PAIR(1);
    try_resize_winmap(win, world.map.length, world.map.length * 2);
    uint16_t x, y, z;
    for (y = 0, z = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            attr_t attr_c = ' ' == world.mem_map[z] ? attr_sha : attr_mem;
            chtype c = world.mem_map[z] | attr_c;
            set_ch_on_yx(win, y, x * 2 + (y % 2), c);
            if (x + (y % 2) < world.map.length)
            {
                set_ch_on_yx(win, y, x * 2 + (y % 2) + 1, ' ' | attr_c);
            }
            z++;
        }
    }
    for (y = 0, z = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            if (' ' != world.map.cells[z])
            {
                chtype c = world.map.cells[z] | attr_fov;
                set_ch_on_yx(win, y, x * 2 + (y % 2), c);
                if (x + (y % 2) < world.map.length)
                {
                    set_ch_on_yx(win, y, x * 2 + (y % 2) + 1, ' ' | attr_fov);
                }
            }
            z++;
        }
    }
}



extern void draw_win_info(struct Win * win)
{
    char * dsc_turn      = "Turn: ";
    char * dsc_hitpoints = "\nHitpoints: ";
    uint16_t maxl = strlen(dsc_turn) + 5 + strlen(dsc_hitpoints) + 3;
    char * text = try_malloc(maxl + 1, __func__);
    int test = sprintf(text, "%s%d%s%d", dsc_turn, world.turn, dsc_hitpoints,
                                         world.player_lifepoints);
    exit_trouble(test < 0, __func__, "sprintf");
    add_text_with_linebreaks(win, text);
    free(text);
}



extern void draw_win_inventory(struct Win * win)
{
    char * limit;
    char * text_copy = strdup(world.player_inventory);
    char * start = text_copy;
    uint8_t i = 0;
    uint8_t found_selection = 0;
    uint16_t offset = 0;
    while (NULL != (limit = strchr(start, '\n')))
    {
        (* limit) = '\0';
        attr_t attri = 0;
        if (i == world.player_inventory_select)
        {
            found_selection = 1;
            attri = A_REVERSE;
            win->center.y = win->winmap_size.y;
        }
        add_line(win, start, attri, &offset, 0);
        start = limit + 1;
        i++;
    }
    win->center.y = !found_selection ? win->winmap_size.y : win->center.y;
    add_line(win, start, !found_selection * A_REVERSE, &offset, 1);
    free(text_copy);
}



extern void draw_win_active_windows_keys(struct Win * win)
{
    struct Win * win_active = get_win_by_id(world.winDB.active);
    struct KeyBindingDB * kbdb = &win_active->kb;
    if      (1 == win_active->view)
    {
        kbdb = &world.kb_wingeom;
    }
    else if (2 == win_active->view)
    {
        kbdb = &world.kb_winkeys;
    }
    uint16_t offset = 0;
    if (0 == kbdb->n_of_kbs)
    {
        add_line(win, "(none)", 0, &offset, 0);
        return;
    }
    uint8_t kb_n;
    for (kb_n = 0; kb_n < kbdb->n_of_kbs; kb_n++)
    {
        char * kb_line = get_kb_line(&kbdb->kbs[kb_n]);
        add_line(win, kb_line, 0, &offset, (0 == kb_n + 1));
        free(kb_line);
    }

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
    char * title = "Window's keys:";
    uint16_t offset = 0;
    add_line(win, title, 0, &offset, 0);
    add_line(win, " ", 0, &offset, 0);
    draw_keybinding_config(win, &win->kb, offset);
}



extern void draw_winconf_geometry(struct Win * win)
{
    char * title = "Window's geometry:\n\n";
    char * h_title = "Height to save: ";
    char h_value[6 + 1];                       /* 6: int16_t value max strlen */
    int test = sprintf(h_value, "%d", win->target_height);
    exit_trouble(test < 0, __func__, "sprintf");
    char * h_plus = " (width in cells)\n\n";
    char * h_minus = " (negative diff: cells to screen width)\n\n";
    char * h_type = (1 == win->target_height_type) ? h_minus : h_plus;
    char * w_title = "Width to save: ";
    char w_value[6 + 1];
    test = sprintf(w_value, "%d", win->target_width);
    exit_trouble(test < 0, __func__, "sprintf");
    char * w_plus = " (height in cells)\n\n";
    char * w_minus = " (negative diff: cells to screen height)\n\n";
    char * w_type = (1 == win->target_width_type)  ? w_minus : w_plus;
    char * breaks_title = "Linebreak type: ";
    char * breaks_type = (1 == win->linebreak) ? "wide" : "long";
    breaks_type        = (2 == win->linebreak) ? "compact" : breaks_type;
    uint16_t text_size =   strlen(title)
                         + strlen(h_title) + strlen(h_value) + strlen(h_type)
                         + strlen(w_title) + strlen(w_value) + strlen(w_type)
                         + strlen(breaks_title) + strlen(breaks_type);
    char * text = try_malloc(text_size + 1, __func__);
    test = sprintf(text, "%s%s%s%s%s%s%s%s%s", title, h_title, h_value, h_type,
                        w_title, w_value, w_type, breaks_title, breaks_type);
    exit_trouble(test < 0, __func__, "sprintf");
    add_text_with_linebreaks(win, text);
    free(text);
}
