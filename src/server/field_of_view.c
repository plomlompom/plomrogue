/* src/server/field_of_view.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "field_of_view.h"
#include <math.h> /* pow() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, int32_t, UINT8_MAX */
#include <stdlib.h> /* free() */
#include <string.h> /* memset() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* yx_uint8 */
#include "map.h" /* mv_yx_in_dir_legal(), init_empty_map() */
#include "rrand.h" /* rrand() */
#include "things.h" /* Thing, ThingInMemory, add_thing_to_memory_map() */
#include "world.h" /* world  */



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



/* Recalculate angle < 0 or > CIRCLE to a value between these two limits. */
static uint32_t correct_angle(int32_t angle);

/* Try merging the angle between "left_angle" and "right_angle" to "shadow" if
 * it meets the shadow from the right or the left. Returns 1 on success, else 0.
 */
static uint8_t try_merge(struct shadow_angle * shadow,
                         uint32_t left_angle, uint32_t right_angle);

/* Try merging the shadow angle between "left_angle" and "right_angle" into an
 * existing shadow angle in "shadows". On success, see if this leads to any
 * additional shadow angle overlaps and merge these accordingly. Return 1 on
 * success, else 0.
 */
static uint8_t try_merging_angles(uint32_t left_angle, uint32_t right_angle,
                                  struct shadow_angle ** shadows);

/* Test whether angle between "left_angle" and "right_angle", or at least
 * "middle_angle", is captured inside one of the shadow angles in "shadows". If
 * so, set hex in "fov_map" indexed by "pos_in_map" to 'H'. If the whole angle
 * and not just "middle_angle" is captured, return 1. Any other case: 0.
 */
static uint8_t shade_hex(uint32_t left_angle, uint32_t right_angle,
                         uint32_t middle_angle, struct shadow_angle ** shadows,
                         uint16_t pos_in_map, char * fov_map);

/* Free shadow angles list "angles". */
static void free_angles(struct shadow_angle * angles);

/* Evaluate map position "test_pos" in distance "dist" to the view origin, and
 * on the circle of that distance to the origin on hex "hex_i" (as counted from
 * the circle's rightmost point), for setting shaded hexes in "fov_map" and
 * potentially adding a new shadow to linked shadow angle list "shadows".
 */
static void eval_position(uint16_t dist, uint16_t hex_i, char * fov_map,
                          struct yx_uint8 * test_pos,
                          struct shadow_angle ** shadows);

/* Update "t_eye"'s things-on-map memory by removing from its .t_mem all
 * memorized thing in FOV, and adding inanimate things in FOV to it.
 */
static void add_things_to_map_memory(struct Thing * t_eye);



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



/* To "shadows", add shadow defined by "left_angle" and "right_angle", either as
 * new entry or as part of an existing shadow (swallowed whole or extending it).
 */
static void set_shadow(uint32_t left_angle, uint32_t right_angle,
                       struct shadow_angle ** shadows)
{
    struct shadow_angle * shadow_i;
    if (!try_merging_angles(left_angle, right_angle, shadows))
    {
        struct shadow_angle * shadow;
        shadow = try_malloc(sizeof(struct shadow_angle), __func__);
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
                    return;
                }
            }
        }
        *shadows = shadow;
    }
}



static void free_angles(struct shadow_angle * angles)
{
    if (angles->next)
    {
        free_angles(angles->next);
    }
    free(angles);
}



static void eval_position(uint16_t dist, uint16_t hex_i, char * fov_map,
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
    uint16_t pos_in_map = test_pos->y * world.map.length + test_pos->x;
    uint8_t all_shaded = shade_hex(left_angle, right_angle_1st, middle_angle,
                                   shadows, pos_in_map, fov_map);
    if (!all_shaded && 'X' == world.map.cells[pos_in_map])
    {
        set_shadow(left_angle, right_angle_1st, shadows);
        if (right_angle_1st != right_angle)
        {
            left_angle = CIRCLE;
            set_shadow(left_angle, right_angle, shadows);
        }
    }
}



static void add_things_to_map_memory(struct Thing * t_eye)
{
    struct ThingInMemory * tm = t_eye->t_mem;
    struct ThingInMemory * tm_prev = NULL;
    struct ThingInMemory * tm_next = NULL;
    for (; tm; tm = tm_next)
    {
        tm_next = tm->next;
        if ('v' == t_eye->fov_map[tm->pos.y * world.map.length + tm->pos.x])
        {
            if (tm_prev)
            {
                tm_prev->next = tm->next;
            }
            else
            {
                t_eye->t_mem = tm->next;
            }
            free(tm);
            continue;
        }
        tm_prev = tm;
    }
    struct Thing * t = world.things;
    for (; t; t = t->next)
    {
        if (   !t->lifepoints
            && 'v' == t_eye->fov_map[t->pos.y * world.map.length + t->pos.x])
        {
            add_thing_to_memory_map(t_eye, t->type, t->pos.y, t->pos.x);
        }
    }
}



extern void update_map_memory(struct Thing * t_eye)
{
    if (!t_eye->mem_map)
    {
        init_empty_map(&(t_eye->mem_map));
    }
    if (!t_eye->mem_depth_map)
    {
        init_empty_map(&(t_eye->mem_depth_map));
    }
    uint32_t i;
    for (i = 0; i < (uint32_t) (world.map.length * world.map.length); i++)
    {
        if ('v' == t_eye->fov_map[i])
        {
            t_eye->mem_depth_map[i] = '0';
            if (' ' == t_eye->mem_map[i])
            {
                t_eye->mem_map[i] = world.map.cells[i];
            }
            continue;
        }
        if (   '0' <= t_eye->mem_depth_map[i] && '9' > t_eye->mem_depth_map[i]
            && !(rrand() % (uint16_t) pow(2, t_eye->mem_depth_map[i] - 48)))
        {
            t_eye->mem_depth_map[i]++;
        }
    }
    add_things_to_map_memory(t_eye);
}



extern void build_fov_map(struct Thing * t)
{
    uint32_t map_size = world.map.length * world.map.length;
    t->fov_map = t->fov_map ? t->fov_map : try_malloc(map_size, __func__);
    memset(t->fov_map, 'v', map_size);
    struct shadow_angle * shadows = NULL;
    struct yx_uint8 test_pos = t->pos;
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
                eval_position(circle_i, hex_i, t->fov_map, &test_pos, &shadows);
                circle_is_on_map = 1;
            }
        }
    }
    mv_yx_in_dir_legal(0, NULL);
    free_angles(shadows);
}
