#include <math.h> /* pow() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* ?(u)int(8|16|32)_t, ?(U)INT8_(MIN|MAX) */
#include <stdlib.h> /* free, malloc */
#include <string.h> /* memset */

/* Number of degrees a circle is divided into. The greater it is, the greater
 * the angle precision. But make it one whole zero larger and bizarre FOV bugs
 * appear on large maps, probably due to value overflows (TODO: more research!).
 */
#define CIRCLE 3600000

/* Angle of a shadow. */
struct shadow_angle
{
    struct shadow_angle * next;
    uint32_t left_angle;
    uint32_t right_angle;
};

/* To be used as temporary storage for world map array. */
static char * worldmap = NULL;

/* Coordinate for maps of max. 256x256 cells. */
struct yx_uint8
{
    uint8_t y;
    uint8_t x;
};

/* Storage for map_length, set by set_maplength(). */
static uint16_t maplength = 0;
extern void set_maplength(uint16_t maplength_input)
{
    maplength = maplength_input;
}

/* Pseudo-randomness seed for rrand(), set by seed_rrand(). */
static uint32_t seed = 0;

/* Helper to mv_yx_in_dir_legal(). Move "yx" into hex direction "d". */
static void mv_yx_in_dir(char d, struct yx_uint8 * yx)
{
    if      (d == 'e')
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
}

/* Move "yx" into hex direction "dir". Available hex directions are: 'e'
 * (north-east), 'd' (east), 'c' (south-east), 'x' (south-west), 's' (west), 'w'
 * (north-west). Returns 1 if the move was legal, 0 if not, and -1 when internal
 * wrapping limits were exceeded.
 *
 * A move is legal if "yx" ends up within the the map and the original wrap
 * space. The latter is left to a neighbor wrap space if "yx" moves beyond the
 * minimal (0) or maximal (UINT8_MAX) column or row of possible map space – in
 * which case "yx".y or "yx".x will snap to the respective opposite side. The
 * current wrapping state is kept between successive calls until a "yx" of NULL
 * is passed, in which case the function does nothing but zero the wrap state.
 * Successive wrapping may move "yx" several wrap spaces into either direction,
 * or return it into the original wrap space.
 */
static int8_t mv_yx_in_dir_legal(char dir, struct yx_uint8 * yx)
{
    static int8_t wrap_west_east   = 0;
    static int8_t wrap_north_south = 0;
    if (!yx)
    {
        wrap_west_east = wrap_north_south = 0;
        return 0;
    }
    if (   INT8_MIN == wrap_west_east || INT8_MIN == wrap_north_south
        || INT8_MAX == wrap_west_east || INT8_MAX == wrap_north_south)
    {
        return -1;
    }
    struct yx_uint8 original = *yx;
    mv_yx_in_dir(dir, yx);
    if      (('e' == dir || 'd' == dir || 'c' == dir) && yx->x < original.x)
    {
        wrap_west_east++;
    }
    else if (('x' == dir || 's' == dir || 'w' == dir) && yx->x > original.x)
    {
        wrap_west_east--;
    }
    if      (('w' == dir || 'e' == dir)               && yx->y > original.y)
    {
        wrap_north_south--;
    }
    else if (('x' == dir || 'c' == dir)               && yx->y < original.y)
    {
        wrap_north_south++;
    }
    if (   !wrap_west_east && !wrap_north_south
        && yx->x < maplength && yx->y < maplength)
    {
        return 1;
    }
    return 0;
}

/* Wrapper around mv_yx_in_dir_legal() that stores new coordinate in res_y/x,
 * (return with result_y/x()), and immediately resets the wrapping.
 */
static uint8_t res_y = 0;
static uint8_t res_x = 0;
extern uint8_t mv_yx_in_dir_legal_wrap(char dir, uint8_t y, uint8_t x)
{
    struct yx_uint8 yx;
    yx.y = y;
    yx.x = x;
    uint8_t result = mv_yx_in_dir_legal(dir, &yx);
    mv_yx_in_dir_legal(0, NULL);
    res_y = yx.y;
    res_x = yx.x;
    return result;
}
extern uint8_t result_y()
{
    return res_y;
}
extern uint8_t result_x()
{
    return res_x;
}

/* With set_seed set, set seed global to seed_input. In any case, return it. */
extern uint32_t seed_rrand(uint8_t set_seed, uint32_t seed_input)
{
    if (set_seed)
    {
        seed = seed_input;
    }
    return seed;
}

/* Return 16-bit number pseudo-randomly generated via Linear Congruential
 * Generator algorithm with some proven constants. Use instead of any rand() to
  * ensure portability of the same pseudo-randomness across systems.
 */
extern uint16_t rrand()
{   /* Constants as recommended by POSIX.1-2001 (see man page rand(3)). */
    seed = ((seed * 1103515245) + 12345) % 4294967296;
    return (seed >> 16); /* Ignore less random least significant bits. */
}

/* Free shadow angles list "angles". */
static void free_angles(struct shadow_angle * angles)
{
    if (angles->next)
    {
        free_angles(angles->next);
    }
    free(angles);
}

/* Recalculate angle < 0 or > CIRCLE to a value between these two limits. */
static uint32_t correct_angle(int32_t angle)
{
    while (angle < 0)
    {
        angle = angle + CIRCLE;
    }
    while (angle > CIRCLE)
    {
        angle = angle - CIRCLE;
    }
    return angle;
}

/* Try merging the angle between "left_angle" and "right_angle" to "shadow" if
 * it meets the shadow from the right or the left. Returns 1 on success, else 0.
 */
static uint8_t try_merge(struct shadow_angle * shadow,
                         uint32_t left_angle, uint32_t right_angle)
{
    if      (   shadow->right_angle <= left_angle + 1
             && shadow->right_angle >= right_angle)
    {
        shadow->right_angle = right_angle;
    }
    else if (   shadow->left_angle + 1 >= right_angle
             && shadow->left_angle     <= left_angle)
    {
        shadow->left_angle = left_angle;
    }
    else
    {
        return 0;
    }
    return 1;
}

/* Try merging the shadow angle between "left_angle" and "right_angle" into an
 * existing shadow angle in "shadows". On success, see if this leads to any
 * additional shadow angle overlaps and merge these accordingly. Return 1 on
 * success, else 0.
 */
static uint8_t try_merging_angles(uint32_t left_angle, uint32_t right_angle,
                                  struct shadow_angle ** shadows)
{
    uint8_t angle_merge = 0;
    struct shadow_angle * shadow;
    for (shadow = *shadows; shadow; shadow = shadow->next)
    {
        if (try_merge(shadow, left_angle, right_angle))
        {
            angle_merge = 1;
        }
    }
    if (angle_merge)
    {
        struct shadow_angle * shadow1;
        for (shadow1 = *shadows; shadow1; shadow1 = shadow1->next)
        {
            struct shadow_angle * last_shadow = NULL;
            struct shadow_angle * shadow2;
            for (shadow2 = *shadows; shadow2; shadow2 = shadow2->next)
            {
                if (   shadow1 != shadow2
                    && try_merge(shadow1, shadow2->left_angle,
                                          shadow2->right_angle))
                {
                    struct shadow_angle * to_free = shadow2;
                    if (last_shadow)
                    {
                        last_shadow->next = shadow2->next;
                        shadow2 = last_shadow;
                    }
                    else
                    {
                        *shadows = shadow2->next;
                        shadow2 = *shadows;
                    }
                    free(to_free);
                }
                last_shadow = shadow2;
            }
        }
    }
    return angle_merge;
}

/* To "shadows", add shadow defined by "left_angle" and "right_angle", either as
 * new entry or as part of an existing shadow (swallowed whole or extending it).
 * Return 1 on malloc error, else 0.
 */
static uint8_t set_shadow(uint32_t left_angle, uint32_t right_angle,
                          struct shadow_angle ** shadows)
{
    struct shadow_angle * shadow_i;
    if (!try_merging_angles(left_angle, right_angle, shadows))
    {
        struct shadow_angle * shadow;
        shadow = malloc(sizeof(struct shadow_angle));
        if (!shadow)
        {
            return 1;
        }
        shadow->left_angle  = left_angle;
        shadow->right_angle = right_angle;
        shadow->next = NULL;
        if (*shadows)
        {
            for (shadow_i = *shadows; shadow_i; shadow_i = shadow_i->next)
            {
                if (!shadow_i->next)
                {
                    shadow_i->next = shadow;
                    return 0;
                }
            }
        }
        *shadows = shadow;
    }
    return 0;
}

/* Test whether angle between "left_angle" and "right_angle", or at least
 * "middle_angle", is captured inside one of the shadow angles in "shadows". If
 * so, set hex in "fov_map" indexed by "pos_in_map" to 'H'. If the whole angle
 * and not just "middle_angle" is captured, return 1. Any other case: 0.
 */
static uint8_t shade_hex(uint32_t left_angle, uint32_t right_angle,
                         uint32_t middle_angle, struct shadow_angle ** shadows,
                         uint16_t pos_in_map, char * fov_map)
{
    struct shadow_angle * shadow_i;
    if (fov_map[pos_in_map] == 'v')
    {
        for (shadow_i = *shadows; shadow_i; shadow_i = shadow_i->next)
        {
            if (   left_angle <=  shadow_i->left_angle
                && right_angle >= shadow_i->right_angle)
            {
                fov_map[pos_in_map] = 'H';
                return 1;
            }
            if (   middle_angle < shadow_i->left_angle
                && middle_angle > shadow_i->right_angle)
            {
                fov_map[pos_in_map] = 'H';
            }
        }
    }
    return 0;
}

/* Evaluate map position "test_pos" in distance "dist" to the view origin, and
 * on the circle of that distance to the origin on hex "hex_i" (as counted from
 * the circle's rightmost point), for setting shaded hexes in "fov_map" and
 * potentially adding a new shadow to linked shadow angle list "shadows".
 * Return 1 on malloc error, else 0.
 */
static uint8_t eval_position(uint16_t dist, uint16_t hex_i, char * fov_map,
                             struct yx_uint8 * test_pos,
                             struct shadow_angle ** shadows)
{
    int32_t left_angle_uncorrected =   ((CIRCLE / 12) / dist)
                                     - (hex_i * (CIRCLE / 6) / dist);
    int32_t right_angle_uncorrected =   left_angle_uncorrected
                                      - (CIRCLE / (6 * dist));
    uint32_t left_angle  = correct_angle(left_angle_uncorrected);
    uint32_t right_angle = correct_angle(right_angle_uncorrected);
    uint32_t right_angle_1st = right_angle > left_angle ? 0 : right_angle;
    uint32_t middle_angle = 0;
    if (right_angle_1st)
    {
        middle_angle = right_angle + ((left_angle - right_angle) / 2);
    }
    uint16_t pos_in_map = test_pos->y * maplength + test_pos->x;
    uint8_t all_shaded = shade_hex(left_angle, right_angle_1st, middle_angle,
                                   shadows, pos_in_map, fov_map);
    if (!all_shaded && 'X' == worldmap[pos_in_map])
    {
        if (set_shadow(left_angle, right_angle_1st, shadows))
        {
            return 1;
        }
        if (right_angle_1st != right_angle)
        {
            left_angle = CIRCLE;
            if (set_shadow(left_angle, right_angle, shadows))
            {
                return 1;
            }
        }
    }
    return 0;
}

/* Update field of view in "fovmap" of "worldmap_input" as seen from "y"/"x".
 * Return 1 on malloc error, else 0.
 */
extern uint8_t build_fov_map(uint8_t y, uint8_t x,
                             char * fovmap, char * worldmap_input)
{
    worldmap = worldmap_input;
    struct shadow_angle * shadows = NULL;
    struct yx_uint8 test_pos;
    test_pos.y = y;
    test_pos.x = x;
    char * circledirs_string = "xswedc";
    uint16_t circle_i;
    uint8_t circle_is_on_map;
    for (circle_i = 1, circle_is_on_map = 1; circle_is_on_map; circle_i++)
    {
        circle_is_on_map = 0;
        if (1 < circle_i)                      /* All circles but the 1st are */
        {                                      /* moved into starting from a  */
            mv_yx_in_dir_legal('c', &test_pos);/* previous circle's last hex, */
        }                                      /* i.e. from the upper left.   */
        char dir_char = 'd'; /* Circle's 1st hex is entered by rightward move.*/
        uint8_t dir_char_pos_in_circledirs_string = UINT8_MAX;
        uint16_t dist_i, hex_i;
        for (hex_i=0, dist_i=circle_i; hex_i < 6 * circle_i; dist_i++, hex_i++)
        {
            if (circle_i < dist_i)
            {
                dist_i = 1;
                dir_char=circledirs_string[++dir_char_pos_in_circledirs_string];
            }
            if (mv_yx_in_dir_legal(dir_char, &test_pos))
            {
                if (eval_position(circle_i, hex_i, fovmap, &test_pos, &shadows))
                {
                    return 1;
                }
                circle_is_on_map = 1;
            }
        }
    }
    mv_yx_in_dir_legal(0, NULL);
    free_angles(shadows);
    return 0;
}

static uint16_t * score_map = NULL;
static uint16_t neighbor_scores[6];

/* Init AI score map. Return 1 on failure, else 0. */
extern uint8_t init_score_map()
{
    uint32_t map_size = maplength * maplength;
    score_map = malloc(map_size * sizeof(uint16_t));
    if (!score_map)
    {
        return 1;
    }
    uint32_t i = 0;
    for (; i < map_size; i++)
    {
        score_map[i] = UINT16_MAX;
    }
    return 0;
}

/* Set score_map[pos] to score. Return 1 on failure, else 0. */
extern uint8_t set_map_score(uint16_t pos, uint16_t score)
{
    if (!score_map)
    {
        return 1;
    }
    score_map[pos] = score;
    return 0;
}

/* Get score_map[pos]. Return uint16_t value on success, -1 on failure. */
extern int32_t get_map_score(uint16_t pos)
{
    if (!score_map)
    {
        return -1;
    }
    return score_map[pos];
}

/* Free score_map. */
extern void free_score_map()
{
    free(score_map);
    score_map = NULL;
}

/* Write into "neighbors" scores of the immediate neighbors of the score_map
 * cell at pos_i (array index), as found in the directions north-east, east,
 * south-east etc. (clockwise order). Use kill_score for illegal neighborhoods
 * (i.e. if direction would lead beyond the map's border).
 */
static void get_neighbor_scores(uint16_t pos_i, uint16_t kill_score,
                                uint16_t * neighbors)
{
    uint32_t map_size = maplength * maplength;
    uint8_t open_north     = pos_i >= maplength;
    uint8_t open_east      = pos_i + 1 % maplength;
    uint8_t open_south     = pos_i + maplength < map_size;
    uint8_t open_west      = pos_i % maplength;
    uint8_t is_indented    = (pos_i / maplength) % 2;
    uint8_t open_diag_west = is_indented || open_west;
    uint8_t open_diag_east = !is_indented || open_east;
    neighbors[0] = !(open_north && open_diag_east) ? kill_score :
                   score_map[pos_i - maplength + is_indented];
    neighbors[1] = !(open_east) ? kill_score : score_map[pos_i + 1];
    neighbors[2] = !(open_south && open_diag_east) ? kill_score :
                   score_map[pos_i + maplength + is_indented];
    neighbors[3] = !(open_south && open_diag_west) ? kill_score :
                   score_map[pos_i + maplength - !is_indented];
    neighbors[4] = !(open_west) ? kill_score : score_map[pos_i - 1];
    neighbors[5] = !(open_north && open_diag_west) ? kill_score :
                   score_map[pos_i - maplength - !is_indented];
}

/* Call get_neighbor_scores() on neighbor_scores buffer. Return 1 on error. */
extern uint8_t ready_neighbor_scores(uint16_t pos)
{
    if (!score_map)
    {
        return 1;
    }
    get_neighbor_scores(pos, UINT16_MAX, neighbor_scores);
    return 0;
}

/* Return i-th position from neighbor_scores buffer.*/
extern uint16_t get_neighbor_score(uint8_t i)
{
    return neighbor_scores[i];
}

/* Iterate over scored cells in score_map geometry. Compare each cell's score
 * against the score of its immediate neighbors in 6 directions. If any
 * neighbor's score is at least two points lower than the current cell's score,
 * re-set it to 1 point higher than its lowest-scored neighbor. Repeat this
 * whole process until all cells have settled on their final score. Ignore cells
 * whose score is greater than UINT16_MAX - 1 (treat those as unreachable). Also
 * ignore cells whose score is smaller or equal the number of past iterations.
 * Return 1 on error, else 0.
 */
extern uint8_t dijkstra_map()
{
    if (!score_map)
    {
        return 1;
    }
    uint16_t max_score = UINT16_MAX - 1;
    uint32_t map_size = maplength * maplength;
    uint32_t pos;
    uint16_t i_scans, neighbors[6], min_neighbor;
    uint8_t scores_still_changing = 1;
    uint8_t i_dirs;
    for (i_scans = 0; scores_still_changing; i_scans++)
    {
        scores_still_changing = 0;
        for (pos = 0; pos < map_size; pos++)
        {
            uint16_t score = score_map[pos];
            if (score <= max_score && score > i_scans)
            {
                get_neighbor_scores(pos, max_score, neighbors);
                min_neighbor = max_score;
                for (i_dirs = 0; i_dirs < 6; i_dirs++)
                {
                    if (min_neighbor > neighbors[i_dirs])
                    {
                        min_neighbor = neighbors[i_dirs];
                    }
                }
                if (score_map[pos] > min_neighbor + 1)
                {
                    score_map[pos] = min_neighbor + 1;
                    scores_still_changing = 1;
                }
            }
        }
    }
    return 0;
}

extern uint8_t zero_score_map_where_char_on_memdepthmap(char c,
                                                        char * memdepthmap)
{
    if (!score_map)
    {
        return 1;
    }
    uint32_t map_size = maplength * maplength;
    uint16_t pos;
    for (pos = 0; pos < map_size; pos++)
    {
        if (c == memdepthmap[pos])
        {
            score_map[pos] = 0;
        }
    }
    return 0;
}

extern void age_some_memdepthmap_on_nonfov_cells(char * memdepthmap,
                                                     char * fovmap)
{
    uint32_t map_size = maplength * maplength;
    uint16_t pos;
    for (pos = 0; pos < map_size; pos++)
    {
        if ('v' != fovmap[pos])
        {
            char c = memdepthmap[pos];
            if( '0' <= c && '9' > c && !(rrand() % (uint16_t) pow(2, c - 48)))
            {
                memdepthmap[pos]++;
            }
        }
    }
}
