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
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX, UINT32_MAX */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* memset(), strcmp(), strchr(), strdup/(), strlen() */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "keybindings.h" /* struct KeyBindingDB, get_keyname_to_keycode() */
#include "windows.h" /* yx_uint16, Win */
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

/* Draw from line "start" on config view for keybindings defined at "kb". */
static void draw_keybinding_config(struct Win * win, struct KeyBindingDB * kbdb,
                                   uint16_t offset);

/* String building helper functions to draw_winconf_geometry(). */
static char * get_keyname_to_command(char * command_name);
static char * winconf_geom_helper(struct Win * win, char axis, char * sep,
                                  char * newlines, char * value_prefix);



/* try_resize_winmap() only realloc's Win->winmap if this is set. Use this to
 * skip inflationary malloc's by successive try_resize_winmap() calls when the
 * memory needed can be pre-calculated.
 */
static uint8_t do_realloc_winmap = 1;



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
    if (do_realloc_winmap)
    {
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
    }
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
        {                        /* after, but within the last line then.     */
            add_line_wide(win, " ", 0);
        }
        add_text_with_linebreaks(win, text);
    }
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
        struct KeyBinding kb = kbdb->kbs[kb_n];
        char * keyname = get_keyname_to_keycode(kb.keycode);
        uint16_t size = strlen(keyname) + 3 + strlen(kb.command->dsc_long) + 1;
        char * kb_line = try_malloc(size, __func__);
        int test = sprintf(kb_line, "%s - %s", keyname, kb.command->dsc_long);
        exit_trouble(test < 0, __func__, "sprintf");
        free(keyname);
        add_line(win, kb_line, attri, &offset, (kbdb->n_of_kbs == kb_n + 1));
        free(kb_line);
    }
}



static char * get_keyname_to_command(char * command_name)
{
    uint8_t i = 0;
    for (i = 0; i < world.kb_wingeom.n_of_kbs; i++)
    {
        if (!strcmp(world.kb_wingeom.kbs[i].command->dsc_short, command_name))
        {
            return get_keyname_to_keycode(world.kb_wingeom.kbs[i].keycode);
        }
    }
    return "UNDEFINED";
}



static char * winconf_geom_helper(struct Win * win, char axis, char * sep,
                                  char * newlines, char * value_prefix)
{
    char * p0 = 'v'==axis? "Height" : "Width";
    char * p1 = " to save (grow/shrink/toggle positivity with ";
    char * p2 = get_keyname_to_command('v'==axis? "grow_v" : "grow_h");
    char * p4 = get_keyname_to_command('v'==axis? "shri_v" : "shri_v");
    char * p6 = get_keyname_to_command('v'==axis? "to_height_t" : "to_width_t");
    char p8[6 + 1];                            /* 6: int16_t value max strlen */
    int test = sprintf(p8,"%d", 'v'==axis?win->target_height:win->target_width);
    exit_trouble(test < 0, __func__, "sprintf");
    char * p9 = " (";
    char * p10 = "in_cells";
    if (1 == ('v'==axis? win->target_height_type : win->target_width_type))
    {
        p10 = "non-positive diff: cells to screen maximum";
    }
    char * p11 = ")";
    uint8_t size =   strlen(p0) + strlen(p1) + strlen(p2) + strlen(sep)
                   + strlen(p4) + strlen(sep) + strlen(p6) +strlen(value_prefix)
                   + strlen(p8) + strlen(p9) + strlen(p10) + strlen(p11)
                   + strlen(newlines) + 1;
    char * msg = try_malloc(size, __func__);
    sprintf(msg, "%s%s%s%s%s%s%s%s%s%s%s%s%s", p0, p1, p2, sep, p4, sep, p6,
            value_prefix, p8, p9, p10, p11, newlines);
    return msg;
}



extern void draw_win_log(struct Win * win)
{
    if (!world.log)
    {
        return;
    }
    uint32_t x, i, n_postbreak_lines;
    for (i = 0, x = 0, n_postbreak_lines = 1; i < strlen(world.log); i++)
    {
        exit_err(i == UINT32_MAX, "Log too large.");
        x++;
        if (x > win->frame_size.x || '\n' == world.log[i])
        {
            n_postbreak_lines++;
            x = 1;
        }
    }
    if (n_postbreak_lines > win->frame_size.y)
    {
        uint32_t size = n_postbreak_lines * (win->frame_size.x + 1);
        win->winmap = try_malloc(sizeof(chtype) * size, __func__);
        for (i = 0; i < size; win->winmap[i] = ' ', i++);
        /* TODO: This should only be done with "long" line break style. */
        do_realloc_winmap = 0;
        draw_text_from_bottom(win, world.log);
        do_realloc_winmap = 1;
        return;
    }
    draw_text_from_bottom(win, world.log);
}



extern void draw_win_map(struct Win * win)
{
    uint16_t x, y;
    init_pair(1, COLOR_WHITE, COLOR_WHITE); //
    init_pair(2, COLOR_BLUE, COLOR_WHITE); //
    init_pair(3, COLOR_RED, COLOR_WHITE); //
    init_pair(4, COLOR_WHITE, COLOR_BLUE); //
    init_pair(5, COLOR_BLUE, COLOR_RED); //
    init_pair(6, COLOR_BLACK, COLOR_RED); //
    init_pair(7, COLOR_BLUE, COLOR_GREEN); //
    init_pair(8, COLOR_BLUE, COLOR_YELLOW); //
    init_pair(9, COLOR_BLACK, COLOR_MAGENTA); //
    init_pair(18, COLOR_WHITE, COLOR_CYAN); //
    init_pair(10, COLOR_BLACK, COLOR_CYAN); //
    init_pair(11, COLOR_WHITE, COLOR_BLACK); //
    init_pair(12, COLOR_BLUE, COLOR_BLACK); //
    init_pair(13, COLOR_RED, COLOR_BLACK); //
    init_pair(14, COLOR_GREEN, COLOR_BLACK); //
    init_pair(15, COLOR_YELLOW, COLOR_BLACK); //
    init_pair(16, COLOR_CYAN, COLOR_BLACK); //
    init_pair(17, COLOR_MAGENTA, COLOR_BLACK); //
    // attr_t attr_mem = COLOR_PAIR(12);
    // attr_t attr_sha = COLOR_PAIR(1);
    attr_t col_unknown = COLOR_PAIR(1); //
    attr_t col_mem = COLOR_PAIR(4); //
    attr_t col_mem_obstacle = COLOR_PAIR(2); //
    attr_t col_mem_altar = COLOR_PAIR(18); //
    try_resize_winmap(win, world.map.length, world.map.length * 2 + 1);
    for (y = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            char c_m = world.mem_map[y * world.map.length + x]; //
            attr_t a = col_mem; //
            if (c_m == ' ') //
            { //
                a = col_unknown; //
            } //
            else if (c_m == 'X' || c_m == '|') //
            { //
                a = col_mem_obstacle; //
            } //
            else if (c_m == '_')
            { //
                a = col_mem_altar; //
            } //
            // attr_t a=' '==world.mem_map[y*world.map.length+x]?attr_sha:attr_mem;
            char c = world.mem_map[y*world.map.length + x];
            set_ch_on_yx(win, y, x * 2 + (y % 2),     c   | a);
            chtype depth = ' ' | a;  //
            if (world.meta_map_0[y * world.map.length + x] == '2')  //
            {  //
                depth = '+' | col_mem; //
            }  //
            set_ch_on_yx(win, y, x * 2 + (y % 2) + 1, depth);
        }
    }
    attr_t col_plant = COLOR_PAIR(11); //
    attr_t col_altar = COLOR_PAIR(10); //
    attr_t col_tool = COLOR_PAIR(16); //
    attr_t col_dirt = COLOR_PAIR(17); //
    attr_t col_unkraut = COLOR_PAIR(14); //
    attr_t col_animal = COLOR_PAIR(6); //
    attr_t col_player = COLOR_PAIR(3); //
    attr_t col_ground = COLOR_PAIR(15); //
    attr_t col_obstacle = COLOR_PAIR(9); //
    attr_t col_health_good = COLOR_PAIR(7); //
    attr_t col_health_middle = COLOR_PAIR(8); //
    attr_t col_health_bad = COLOR_PAIR(5); //
    attr_t col_lumber = COLOR_PAIR(13); //
    attr_t col_water = COLOR_PAIR(12); //
    attr_t col_stack = COLOR_PAIR(11); //
    for (y = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            char c = world.map.cells[y*world.map.length + x]; //
            if (' ' != c) //
            // if (' ' != world.map.cells[y*world.map.length + x])
            {
                attr_t a = col_water; //
                if ('.' == c || ':' == c) //
                { //
                    a = col_ground; //
                } //
                else if ('@' == c) //
                { //
                    a = col_player; //
                } //
                else if ('a' == c || 'd' == c || 'b' == c) //
                { //
                    a = col_animal; //
                } //
                else if ('#' == c) //
                { //
                    a = col_unkraut; //
                } //
                else if ('$' == c) //
                { //
                    a = col_dirt; //
                } //
                else if ('m' == c || '/' == c || '-' == c) //
                { //
                    a = col_tool; //
                } //
                else if ('X' == c || '|' == c) //
                { //
                    a = col_obstacle; //
                } //
                else if ('_' == c) //
                { //
                    a = col_altar; //
                } //
                else if ('%' == c || '*' == c) //
                { //
                    a = col_plant; //
                } //
                else if ('=' == c) //
                { //
                    a = col_lumber; //
                } //
                // char c = world.map.cells[y*world.map.length + x];
                set_ch_on_yx(win, y, x * 2 + (y % 2),     c | a); //
                chtype meta = ' ' | a;  //
                char metamap0val = world.meta_map_0[y*world.map.length+x]; //
                if (metamap0val == '2')  //
                {  //
                    meta = '+' | col_stack;  //
                }  //
                else if (metamap0val != '0' && metamap0val != '1') //
                { //
                    char c = world.meta_map_1[y*world.map.length+x];
                    if (metamap0val == 'a') //
                    { //
                        meta = c | col_health_bad; //
                    } //
                    else if (metamap0val == 'b') //
                    { //
                        meta = c | col_health_middle; //
                    } //
                    else if (metamap0val == 'c') //
                    { //
                        meta = c | col_health_good; //
                    } //
                } //
                set_ch_on_yx(win, y, x * 2 + (y % 2) + 1, meta); //
                // set_ch_on_yx(win, y, x * 2 + (y % 2),     c);
                // set_ch_on_yx(win, y, x * 2 + (y % 2) + 1, ' ');
            }
        }
    }
    if (world.look)
    {
        y = world.look_pos.y;
        x = world.look_pos.x;
        char c = world.map.cells[y * world.map.length + x];
        c = ' ' == c ? world.mem_map[y * world.map.length + x] : c;
        set_ch_on_yx(win, y, x * 2 + (y % 2),     c   | A_REVERSE);
        set_ch_on_yx(win, y, x * 2 + (y % 2) + 1, '?' | A_REVERSE);
    }
}



extern void draw_win_info(struct Win * win)
{
    char * dsc_turn      = "Turn: ";
    char * dsc_hitpoints = "\nHitpoints: ";
    char * dsc_satiation = "\nSatiation: ";
    char * dsc_godsmood  = "\nGod's mood: "; // 7DRL
    char * dsc_godsfavor = "\nGod's favor: "; // 7DRL
    uint16_t maxl =   strlen(dsc_turn) + 5 + strlen(dsc_hitpoints) + 3 //
                    + strlen(dsc_satiation) + 6 + strlen(dsc_godsmood) + 6 //
                    + strlen(dsc_godsfavor) + 6; //
    char * text = try_malloc(maxl + 1, __func__);
    int test = sprintf(text, "%s%d%s%d%s%d%s%d%s%d", dsc_turn, world.turn,
                       dsc_hitpoints, world.player_lifepoints, dsc_satiation,
                       world.player_satiation, dsc_godsmood, world.godsmood, //
                       dsc_godsfavor, world.godsfavor); //
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



extern void draw_win_terrain_stack(struct Win * win)
{
    char * wait_response = "(polling)";
    char * text = world.things_here ? world.things_here : wait_response;
    add_text_with_linebreaks(win, text);
    win->center.y = world.things_here_scroll + (win->frame_size.y / 2);  //
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
    char * sep = "/";
    char * newlines = "\n\n";
    char * value_prefix = "): ";
    char * title = "Window's geometry:\n\nShift up/down with ";
    char * key_shift_b = get_keyname_to_command("shift_b");
    char * key_shift_f = get_keyname_to_command("shift_f");
    char * height = winconf_geom_helper(win, 'v', sep, newlines, value_prefix);
    char * width = winconf_geom_helper(win, 'h', sep, newlines, value_prefix);
    char * breaks_title = "Linebreak type (toggle with ";
    char * key_to_break = get_keyname_to_command("to_break");
    char * breaks_type = "long";
    if (win->linebreak)
    {
        breaks_type = (1 == win->linebreak) ? "wide" : "compact";
    }
    uint16_t text_size =   strlen(title) + strlen(key_shift_b) + strlen(sep)
                         + strlen(key_shift_f) + strlen(newlines)
                         + strlen(height) + strlen(width)
                         + strlen(breaks_title) + strlen(key_to_break)
                         + strlen(value_prefix) + strlen(breaks_type);
    char * text = try_malloc(text_size + 1, __func__);
    int test = sprintf(text, "%s%s%s%s%s%s%s%s%s%s%s", title, key_shift_b, sep,
                       key_shift_f, newlines, height, width, breaks_title,
                       key_to_break, value_prefix, breaks_type);
    free(height);
    free(width);
    exit_trouble(test < 0, __func__, "sprintf");
    add_text_with_linebreaks(win, text);
    free(text);
}
