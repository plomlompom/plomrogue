/* draw_wins.c */

#include "draw_wins.h"
#include <stdlib.h>      /* for malloc(), free() */
#include <stdint.h>      /* for uint16_t */
#include <string.h>      /* for strlen() */
#include <ncurses.h>     /* for mvwaddch() */
#include "windows.h"     /* for structs Win, Frame, for draw_scroll_hint() */
#include "misc.h"        /* for center_offset() */
#include "keybindings.h" /* for struct KeyBinding, for get_keyname() */
#include "map_objects.h" /* for structs MapObj, Player */
#include "map.h"         /* for Map struct */
#include "main.h"        /* for World struct */
#include "rexit.h"       /* for err_exit() */



/* Write "text" into window "win" as far as possible. Start on row "start_y". */
static void draw_with_linebreaks(struct Win * win, char * text,
                                 uint16_t start_y);

/* Write "text" not starting from the top but from the bottom of "win". */
static void draw_text_from_bottom(struct Win * win, char * text);

/* Draw onto "map" in "win" the objects in the chain at "start". */
static void draw_map_objects(struct World * world, struct MapObj * start,
                             struct Map * map, struct Win * win);



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



extern void draw_log_win(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    draw_text_from_bottom(win, world->log);
}



extern void draw_map_win(struct Win * win)
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



extern void draw_info_win(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    char text[100];
    snprintf(text, 100,
             "Turn: %d\nHitpoints: %d", world->turn, world->player->hitpoints);
    draw_with_linebreaks(win, text, 0);
}



extern void draw_keys_win(struct Win * win)
{
    struct World * world = (struct World *) win->data;
    uint16_t offset, y, x;
    offset = center_offset(world->keyswindata->select, world->keyswindata->max,
                           win->frame.size.y - 1);
    uint8_t keydescwidth = 9 + 1; /* max length assured by get_keyname() + \0 */
    char * keydesc = malloc(keydescwidth), * keyname;
    attr_t attri;
    for (y = 0; y <= world->keyswindata->max && y < win->frame.size.y; y++)
    {
        if (0 == y && offset > 0)
        {
            exit_err(draw_scroll_hint(&win->frame, y, offset + 1, '^'),
                     world, NULL);
            continue;
        }
        else if (win->frame.size.y == y + 1
                 && 0 < world->keyswindata->max
                        - (win->frame.size.y + offset - 1))
        {
            exit_err(draw_scroll_hint(&win->frame, y,
                                      world->keyswindata->max
                                       - (offset + win->frame.size.y) + 2, 'v'),
                     world, NULL);
            continue;
        }
        attri = 0;
        if (y == world->keyswindata->select - offset)
        {
            attri = A_REVERSE;
            if (1 == world->keyswindata->edit)
            {
                attri = attri | A_BLINK;
            }
        }
        keyname = get_keyname(world->keybindings[y + offset].key);
        snprintf(keydesc, keydescwidth, "%-9s", keyname);
        free(keyname);
        for (x = 0; x < win->frame.size.x; x++)
        {
            if (x < strlen(keydesc))
            {
                mvwaddch(win->frame.curses_win, y, x, keydesc[x] | attri);
            }
            else if (strlen(keydesc) < x
                     && x < strlen(world->keybindings[y + offset].name)
                            + strlen(keydesc) + 1)
            {
                mvwaddch(win->frame.curses_win, y, x,
                         world->keybindings[y + offset]
                         .name[x - strlen(keydesc) - 1] | attri);
            }
            else
            {
                mvwaddch(win->frame.curses_win, y, x, ' ' | attri);
            }
        }
    }
    free(keydesc);
}
