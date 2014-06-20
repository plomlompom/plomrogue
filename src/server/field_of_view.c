/* src/server/field_of_view.c */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include "field_of_view.h"
#include <stdlib.h> /* free() */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <string.h> /* memset(), strchr(), strdup() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "map.h" /* yx_to_map_pos() */
#include "things.h" /* Thing */
#include "yx_uint8.h" /* yx_uint8 */
#include "world.h" /* global world  */



/* Values for mv_yx_in_dir_wrap()'s wrapping directory memory. */
enum wraps
{
    WRAP_N = 0x01,
    WRAP_S = 0x02,
    WRAP_E = 0x04,
    WRAP_W = 0x08
};

/* Move "yx" into hex direction "d". If this moves "yx" beyond the minimal (0)
 * or maximal (UINT8_MAX) column or row, it wraps to the opposite side. Such
 * wrapping is returned as a wraps enum value and stored, so that further calls
 * to move "yx" back into the opposite direction may unwrap it again. Pass an
 * "unwrap" of UNWRAP to re-set the internal wrap memory to 0.
 */
static uint8_t mv_yx_in_dir_wrap(char d, struct yx_uint8 * yx, uint8_t unwrap);

/* Wrapper to "mv_yx_in_dir_wrap()", returns 1 if the wrapped function moved
 * "yx" within the wrap borders and the map size, else 0.
 */
extern uint8_t mv_yx_in_dir_legal(char dir, struct yx_uint8 * yx);

/* Return one by one hex dir characters of walking through a circle of "radius".
 * The circle is initialized by passing a "new_circle" of 1 and the "radius"
 * and only returns non-null hex direction characters if "new_circle" is 0.
 */
static char next_circle_dir(uint8_t new_circle, uint8_t radius_new);

/* Draw circle of hexes flagged LIMIT "radius" away from "yx" to "fov_map". */
extern void draw_border_circle(struct yx_uint8 yx, uint8_t radius,
                               uint8_t * fov_map);

/* eye_to_cell_dir_ratio() helper. */
static void geometry_to_char_ratio(uint8_t * n1, uint8_t * n2, uint8_t indent,
                                   int16_t diff_y, int16_t diff_x,
                                   uint8_t vertical, uint8_t variant);

/* From the chars in "available_dirs" and the geometry described by the other
 * parameters return a string of hex direction characters representing the
 * approximation of a straight line. "variant" marks the direction as either in
 * the northern, north-eastern or south-western hex neighborhood if 1, or the
 * others if 0.
 */
static char * eye_to_cell_dir_ratio(char * available_dirs, uint8_t indent,
                                    int16_t diff_y, int16_t diff_x,
                                    uint8_t vertical, uint8_t variant,
                                    uint8_t shift_right);

/* Return string approximating in one or two hex direction chars the direction
 * that a "diff_y" and "diff_x" lead to in the internal half-indented 2D
 * encoding of hexagonal maps, with "indent" the movement's start indentation.
 */
static char * dir_from_delta(uint8_t indent, int16_t diff_y, int16_t diff_x);

/* Return string of hex movement direction characters describing the best
 * possible hex approximation to a straight line from "yx_eye" to "yx_cell". If
 * "right" is set and the string is of length two, return it with the direction
 * strings scarcer character appearing first.
 */
static char * eye_to_cell(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_cell,
                          uint8_t right);

/* Return string of hex movement direction characters describing the best
 * possible hex approximation to a straight line from "yx_eye" to "yx_cell". If
 * "right" is set and the string is of length two, return it with the direction
 * strings scarcer character appearing first.
 */
static char * eye_to_cell(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_cell,
                          uint8_t right);

/* fill_shadow() helper, determining if map's top left cell starts a shadow. */
static uint8_t is_top_left_shaded(uint16_t pos_a, uint16_t pos_b,
                                  int16_t a_y_on_left);

/* Flag as HIDDEN all cells in "fov_map" that are enclosed by 1) the map's
 * borders or cells flagged LIMIT and 2) the shadow arms of cells flagged
 * SHADOW_LEFT and SHADOW_RIGHT extending from "yx_cell", as seen as left and
 * right as seen from "yx_eye". "pos_a" and "pos_b" store the terminal positions
 * of these arms in "fov_map" ("pos_a" for the left, "pos_b" for the right one).
 */
static void fill_shadow(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_cell,
                        uint8_t * fov_map, uint16_t pos_a, uint16_t pos_b);

/* Flag with "flag" cells of a path from "yx_start" to the end of the map or (if
 * closer) the view border circle of the cells flagged as LIMIT, in a direction
 * parallel to the one determined by walking a path from "yx_eye" to the cell
 * reachable by moving one step into "dir" from "yx_start". If "shift_right" is
 * set, choose among the possible paths the one whose starting cell is set most
 * to the right, else do the opposite.
 */
static uint16_t shadow_arm(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_start,
                           uint8_t * fov_map, char dir, uint8_t flag,
                           uint8_t shift_right);

/* From "yx_start", draw shadow of what is invisible as seen from "yx_eye" into
 * "fov_map" by extending shadow arms from "yx_start" as shadow borders until
 * the edges of the map or, if smaller, the maximum viewing distance, flag these
 * shadow arms' cells as HIDE_LATER and the area enclosed by them as HIDDEN.
 * "dir_left" and "dir_right" are hex directions to move to from "yx_start" for
 * cells whose shortest straight path to "yx_eye" serve as the lines of sight
 * enclosing the shadow left and right (left and right as seen from "yx_eye").
 */
static void shadow(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_start,
                   uint8_t * fov_map, char dir_left, char dir_right);

/* In "fov_map", if cell of position "yx_cell" is not HIDDEN, set it as VISIBLE,
 * and if an obstacle to view is positioned there in the game map, flag cells
 *behind it, unseen from "yx_eye", as HIDDEN on the interior and HIDE_LATER on
 * their borders.
 *
 * The shape and width of shadows is determined by 1) calculating an approximate
 * direction of "yx_cell" as seen from "yx_eye" as one hex movement direction,
 * or two directly neighboring each other (i.e. "east", "east and north-east"),
 * 2) deriving the two hex movement directions clockwise immediately preceding
 * the first (or only) direction and immediately succeeding the second (or only)
 * one and 3) passing the two directions thus gained as shadow arm direction
 * calibration values to shadow() (after this function's other arguments).
 */
static void set_view_of_cell_and_shadows(struct yx_uint8 * yx_cell,
                                         struct yx_uint8 * yx_eye,
                                         uint8_t * fov_map);



static uint8_t mv_yx_in_dir_wrap(char d, struct yx_uint8 * yx, uint8_t unwrap)
{
    static uint8_t wrap = 0;
    if (unwrap)
    {
        wrap = 0;
        return 0;
    }
    struct yx_uint8 original;
    original.y = yx->y;
    original.x = yx->x;
    if     (d == 'e')
    {
        yx->x = yx->x + (yx->y % 2);
        yx->y--;
    }
    else if (d == 'd')
    {
        yx->x++;
    }
    else if (d == 'c')
    {
        yx->x = yx->x + (yx->y % 2);
        yx->y++;
    }
    else if (d == 'x')
    {
        yx->x = yx->x - !(yx->y % 2);
        yx->y++;
    }
    else if (d == 's')
    {
        yx->x--;
    }
    else if (d == 'w')
    {
        yx->x = yx->x - !(yx->y % 2);
        yx->y--;
    }
    else
    {
        exit_trouble(1, "mv_yx_in_dir_wrap()", "illegal direction");
    }
    if      (strchr("edc", d) && yx->x < original.x)
    {
        wrap = wrap & WRAP_W ? wrap ^ WRAP_W : wrap | WRAP_E;
    }
    else if (strchr("xsw", d) && yx->x > original.x)
    {
        wrap = wrap & WRAP_E ? wrap ^ WRAP_E : wrap | WRAP_W;
    }
    if      (strchr("we", d) && yx->y > original.y)
    {
        wrap = wrap & WRAP_S ? wrap ^ WRAP_S : wrap | WRAP_N;
    }
    else if (strchr("xc", d) && yx->y < original.y)
    {
        wrap = wrap & WRAP_N ? wrap ^ WRAP_N : wrap | WRAP_S;
    }
    return wrap;
}



extern uint8_t mv_yx_in_dir_legal(char dir, struct yx_uint8 * yx)
{
    uint8_t wraptest = mv_yx_in_dir_wrap(dir, yx, 0);
    if (!wraptest && yx->x < world.map.length && yx->y < world.map.length)
    {
        return 1;
    }
    return 0;
}



static char next_circle_dir(uint8_t new_circle, uint8_t radius_new)
{
    static uint8_t i_dirs = 0;
    static uint8_t i_dist = 0;
    static uint8_t radius = 0;
    char * dirs = "dcxswe";
    if (new_circle)
    {
        i_dirs = 0;
        i_dist = 0;
        radius = radius_new;
        return '\0';
    }
    char ret_dir = dirs[i_dirs];
    i_dist++;
    if (i_dist == radius)
    {
        i_dist = 0;
        i_dirs++;
    }
    return ret_dir;
}



extern void draw_border_circle(struct yx_uint8 yx, uint8_t radius,
                               uint8_t * fov_map)
{
    uint8_t dist;
    for (dist = 1; dist <= radius; dist++)
    {
        mv_yx_in_dir_wrap('w', &yx, 0);
    }
    next_circle_dir(1, radius);
    char dir;
    while ('\0' != (dir = next_circle_dir(0, 0)))
    {
         if (mv_yx_in_dir_legal(dir, &yx))
         {
            uint16_t pos = yx_to_map_pos(&yx);
            fov_map[pos] = LIMIT;
        }
    }
    mv_yx_in_dir_wrap(0, NULL, 1);
}



static void geometry_to_char_ratio(uint8_t * n1, uint8_t * n2, uint8_t indent,
                                   int16_t diff_y, int16_t diff_x,
                                   uint8_t vertical, uint8_t variant)
{
    if      (vertical)
    {
        *n1 = (diff_y / 2) - diff_x + ( indent * (diff_y % 2));
        *n2 = (diff_y / 2) + diff_x + (!indent * (diff_y % 2));
    }
    else if (!vertical)
    {
        *n1 = diff_y;
        *n2 = diff_x - (diff_y / 2) - (indent * (diff_y % 2));
    }
    if (!variant)
    {
        uint8_t tmp = *n1;
        *n1 = *n2;
        *n2 = tmp;
    }
}



static char * eye_to_cell_dir_ratio(char * available_dirs, uint8_t indent,
                                    int16_t diff_y, int16_t diff_x,
                                    uint8_t vertical, uint8_t variant,
                                    uint8_t shift_right)
{
    char * f_name = "eye_to_cell_dir_ratio()";
    uint8_t n1, n2;
    geometry_to_char_ratio(&n1, &n2, indent, diff_y, diff_x, vertical, variant);
    uint8_t size_chars = n1 + n2;
    char * dirs = try_malloc(size_chars + 1, f_name);
    uint8_t n_strong_char = n1 / n2;
    uint8_t more_char1 = 0 < n_strong_char;
    n_strong_char = !more_char1 ? (n2 / n1) : n_strong_char;
    uint16_t i, i_alter;
    uint8_t i_of_char = shift_right;
    for (i = 0, i_alter = 0; i < size_chars; i++)
    {
        char dirchar = available_dirs[i_of_char];
        if (more_char1 != i_of_char)
        {
            i_alter++;
            if (i_alter == n_strong_char)
            {
                i_alter = 0;
                i_of_char = !i_of_char;
            }
        }
        else
        {
            i_of_char = !i_of_char;
        }

        dirs[i] = dirchar;
    }
    dirs[i] = '\0';
    return dirs;
}



static char * dir_from_delta(uint8_t indent, int16_t diff_y, int16_t diff_x)
{
    int16_t double_x = 2 * diff_x;
    int16_t indent_corrected_double_x_pos =  double_x - indent  + !indent;
    int16_t indent_corrected_double_x_neg = -double_x - !indent +  indent;
    if (diff_y > 0)
    {
        if (diff_y ==  double_x || diff_y == indent_corrected_double_x_pos)
        {
            return "c";
        }
        if (diff_y == -double_x || diff_y == indent_corrected_double_x_neg)
        {
            return "x";
        }
        if (diff_y  <  double_x || diff_y  < indent_corrected_double_x_pos)
        {
            return "dc";
        }
        if (diff_y  < -double_x || diff_y  < indent_corrected_double_x_neg)
        {
            return "xs";
        }
        return "cx";
    }
    if (diff_y < 0)
    {
        if (diff_y ==  double_x || diff_y == indent_corrected_double_x_pos)
        {
            return "w";
        }
        if (diff_y == -double_x || diff_y == indent_corrected_double_x_neg)
        {
            return "e";
        }
        if (diff_y  >  double_x || diff_y  > indent_corrected_double_x_pos)
        {
            return "sw";
        }
        if (diff_y  > -double_x || diff_y  > indent_corrected_double_x_neg)
        {
            return "ed";
        }
        return "we";
    }
    return 0 > diff_x ? "s" : "d";
}



static char * eye_to_cell(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_cell,
                          uint8_t right)
{
    int16_t diff_y = yx_cell->y - yx_eye->y;
    int16_t diff_x = yx_cell->x - yx_eye->x;
    uint8_t indent = yx_eye->y % 2;
    char * dir = dir_from_delta(indent, diff_y, diff_x);
    char * dirs = NULL;
    if (1 == strlen(dir))
    {
        return strdup(dir);
    }
    else if (!strcmp(dir, "dc"))
    {
        dirs = eye_to_cell_dir_ratio(dir, indent,  diff_y,  diff_x,  0,0,right);
    }
    else if (!strcmp(dir, "xs"))
    {
        dirs = eye_to_cell_dir_ratio(dir, !indent,  diff_y, -diff_x, 0,1,right);
    }
    else if (!strcmp(dir, "cx"))
    {
        dirs = eye_to_cell_dir_ratio(dir, indent,  diff_y,  diff_x,  1,0,right);
    }
    else if (!strcmp(dir, "sw"))
    {
        dirs = eye_to_cell_dir_ratio(dir, !indent, -diff_y, -diff_x, 0,0,right);
    }
    else if (!strcmp(dir, "ed"))
    {
        dirs = eye_to_cell_dir_ratio(dir, indent, -diff_y,  diff_x, 0,1,right);
    }
    else if (!strcmp(dir, "we"))
    {
        dirs = eye_to_cell_dir_ratio(dir, indent, -diff_y,  diff_x, 1,1,right);
    }
    return dirs;
}



static uint8_t is_top_left_shaded(uint16_t pos_a, uint16_t pos_b,
                                  int16_t a_y_on_left)
{
    uint16_t start_last_row = world.map.length * (world.map.length - 1);
    uint8_t a_on_left_or_bottom =    0 <= a_y_on_left
                                  || (pos_a >= start_last_row);
    uint8_t b_on_top_or_right =    pos_b < world.map.length
                                || pos_b % world.map.length==world.map.length-1;
    return pos_a != pos_b && b_on_top_or_right && a_on_left_or_bottom;
}



static void fill_shadow(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_cell,
                        uint8_t * fov_map, uint16_t pos_a, uint16_t pos_b)
{
    int16_t a_y_on_left = !(pos_a%world.map.length)? pos_a/world.map.length :-1;
    int16_t b_y_on_left = !(pos_b%world.map.length)? pos_b/world.map.length :-1;
    uint8_t top_left_shaded = is_top_left_shaded(pos_a, pos_b, a_y_on_left);
    uint16_t pos;
    uint8_t y, x, in_shade;
    for (y = 0; y < world.map.length; y++)
    {
        in_shade =    (top_left_shaded || (b_y_on_left >= 0 && y > b_y_on_left))
                   && (a_y_on_left < 0 || y < a_y_on_left);
        for (x = 0; x < world.map.length; x++)
        {
            pos = (y * world.map.length) + x;
            if (yx_eye->y == yx_cell->y && yx_eye->x < yx_cell->x)
            {
                uint8_t val = fov_map[pos] & (SHADOW_LEFT | SHADOW_RIGHT);
                in_shade = 0 < val ? 1 : in_shade;
            }
            else if (yx_eye->y == yx_cell->y && yx_eye->x > yx_cell->x)
            {
                uint8_t val = fov_map[pos] & (SHADOW_LEFT | SHADOW_RIGHT);
                in_shade = 0 < val ? 0 : in_shade;
            }
            else if (yx_eye->y > yx_cell->y && y <= yx_cell->y)
            {
                in_shade = 0 < (fov_map[pos] & SHADOW_LEFT) ? 1 : in_shade;
                in_shade = (fov_map[pos] & SHADOW_RIGHT) ? 0 : in_shade;
            }
            else if (yx_eye->y < yx_cell->y && y >= yx_cell->y)
            {
                in_shade = 0 < (fov_map[pos] & SHADOW_RIGHT) ? 1 : in_shade;
                in_shade = (fov_map[pos] & SHADOW_LEFT) ? 0 : in_shade;
            }
            if (!(fov_map[pos] & (SHADOW_LEFT | SHADOW_RIGHT))
                && in_shade)
            {
                fov_map[pos] = fov_map[pos] | HIDDEN;
            }
        }
    }
}



static uint16_t shadow_arm(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_start,
                           uint8_t * fov_map, char dir, uint8_t flag,
                           uint8_t shift_right)
{
    struct yx_uint8 yx_border = *yx_start;
    uint16_t pos = yx_to_map_pos(&yx_border);
    if (mv_yx_in_dir_legal(dir, &yx_border))
    {
        uint8_t met_limit = 0;
        uint8_t i_dirs = 0;
        char * dirs = eye_to_cell(yx_eye, &yx_border, shift_right);
        yx_border = *yx_start;
        while (!met_limit && mv_yx_in_dir_legal(dirs[i_dirs], &yx_border))
        {
            pos = yx_to_map_pos(&yx_border);
            met_limit = fov_map[pos] & LIMIT;
            fov_map[pos] = fov_map[pos] | flag;
            i_dirs = dirs[i_dirs + 1] ? i_dirs + 1 : 0;
        }
        free(dirs);
    }
    mv_yx_in_dir_wrap(0, NULL, 1);
    return pos;
}



static void shadow(struct yx_uint8 * yx_eye, struct yx_uint8 * yx_start,
                   uint8_t * fov_map, char dir_left, char dir_right)
{
    uint16_t pos_a, pos_b, pos_start, i;
    pos_a = shadow_arm(yx_eye, yx_start, fov_map, dir_left, SHADOW_LEFT, 0);
    pos_b = shadow_arm(yx_eye, yx_start, fov_map, dir_right, SHADOW_RIGHT, 1);
    pos_start = yx_to_map_pos(yx_start);
    fov_map[pos_start] = fov_map[pos_start] | SHADOW_LEFT | SHADOW_RIGHT;
    fill_shadow(yx_eye, yx_start, fov_map, pos_a, pos_b);
    for (i = 0; i < world.map.length * world.map.length; i++)
    {
        if (fov_map[i] & (SHADOW_LEFT | SHADOW_RIGHT) && i != pos_start)
        {
            fov_map[i] = fov_map[i] | HIDE_LATER;
        }
        fov_map[i] = fov_map[i] ^ (fov_map[i] & SHADOW_LEFT);
        fov_map[i] = fov_map[i] ^ (fov_map[i] & SHADOW_RIGHT);
    }
    return;
}



static void set_view_of_cell_and_shadows(struct yx_uint8 * yx_cell,
                                         struct yx_uint8 * yx_eye,
                                         uint8_t * fov_map)
{
    char * dirs = "dcxswe";
    uint16_t pos = yx_to_map_pos(yx_cell);
    if (!(fov_map[pos] & HIDDEN))
    {
        fov_map[pos] = fov_map[pos] | VISIBLE;
        if ('X' == world.map.cells[pos])
        {
            uint8_t last_pos = strlen(dirs) - 1;
            int16_t diff_y = yx_cell->y - yx_eye->y;
            int16_t diff_x = yx_cell->x - yx_eye->x;
            uint8_t indent = yx_eye->y % 2;
            char * dir = dir_from_delta(indent, diff_y, diff_x);
            uint8_t start_pos = strchr(dirs, dir[0]) - dirs;
            char prev = start_pos > 0 ? dirs[start_pos - 1] : dirs[last_pos];
            char next = start_pos < last_pos ? dirs[start_pos + 1] : dirs[0];
            if (dir[1])
            {
                uint8_t end_pos = strchr(dirs, dir[1]) - dirs;
                next = end_pos < last_pos ? dirs[end_pos + 1] : dirs[0];
            }
            shadow(yx_eye, yx_cell, fov_map, prev, next);
        }
    }
}



extern uint8_t * build_fov_map(struct Thing * eye)
{
    char * f_name = "build_fov_map()";
    uint8_t radius = 2 * world.map.length;
    uint32_t map_size = world.map.length * world.map.length;
    struct yx_uint8 yx = eye->pos;
    uint8_t * fov_map = try_malloc(map_size, f_name);
    memset(fov_map, 0, map_size);
    draw_border_circle(yx, radius, fov_map);
    fov_map[yx_to_map_pos(&yx)] = VISIBLE;
    uint8_t dist;
    for (dist = 1; dist <= radius; dist++)
    {
        uint8_t first_round = 1;
        char dir;
        next_circle_dir(1, dist);
        while ('\0' != (dir = next_circle_dir(0, 0)))
        {
            char i_dir = first_round ? 'e' : dir;
            first_round = 0;
            if (mv_yx_in_dir_legal(i_dir, &yx))
            {
                set_view_of_cell_and_shadows(&yx, &eye->pos, fov_map);
            }
        }
    }
    uint16_t i;
    for (i = 0; i < world.map.length * world.map.length; i++)
    {
        if (fov_map[i] & HIDE_LATER)
        {
              fov_map[i] = fov_map[i] ^ (fov_map[i] & VISIBLE);
        }
    }
    return fov_map;
}
