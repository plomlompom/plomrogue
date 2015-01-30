/* src/server/ai.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "ai.h"
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, int16_t, UINT16_MAX */
#include <stdlib.h> /* free() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "hardcoded_strings.h" /* s */
#include "thing_actions.h" /* get_thing_action_id_by_name() */
#include "things.h" /* Thing, ThingType, ThingInMemory */
#include "world.h" /* world */



#define N_DIRS 6



/* Write into "neighbors" scores of the N_DIRS immediate neighbors of the
 * "score_map" cell at "pos_i" (array index), as found in the directions
 * north-east, east, south-east etc. (clockwise order). Use "max_score" for
 * illegal neighborhoods (i.e. if direction would lead beyond the map's border).
 */
static void get_neighbor_scores(uint16_t * score_map, uint16_t pos_i,
                                uint16_t max_score, uint16_t * neighbors);

/* Iterate over scored cells in "score_map" of world.map's geometry. Compare
 * each cell's score against the score of its immediate neighbors in N_DIRS
 * directions. If any neighbor's score is at least two points lower than the
 * current cell's score, re-set it to 1 point higher than its lowest-scored
 * neighbor. Repeat this whole process until all cells have settled on their
 * final score. Ignore cells whose score is greater than "max_score". Expect
 * "max_score" to be the maximum score for cells, marking them as unreachable.
 */
static void dijkstra_map(uint16_t * score_map, uint16_t max_score);

/* get_dir_to_nearest_target() helper: Prepare "score_map" for dijkstra_map(). */
static void init_score_map(char filter, uint16_t * score_map, uint32_t map_size,
                           struct Thing * t_eye);

/* Set (if possible) as "t_eye"'s command a move to the path to the path-wise
 * nearest target that is not "t_eye" and fits criteria set by "filter". On
 * success, return !0, else 0. Values for "filter":
 * "e": thing in FOV is animate, but not of "t_eye"'s thing type; build path as
 *      avoiding things of "t_eye"'s type
 * "c": thing in memorized map is consumable
 * "s": memory map cell with greatest-reachable degree of unexploredness
 */
static uint8_t get_dir_to_nearest_target(struct Thing * t_eye, char filter);

/* Return 1 if any thing not "t_eye" is known and fulfills some criteria defined
 * by "filter", else 0. Values for "filter":
 * "e": thing in FOV is animate, but not of "t_eye"'s thing type
 * "c": thing in memorized map is consumable
 */
static uint8_t seeing_thing(struct Thing * t_eye, char filter);

/* Return slot ID of strongest consumable in "t_owner"'s inventory, else -1. */
static int16_t get_inventory_slot_to_consume(struct Thing * t_owner);

/* Return 1 if "t_standing" is standing on a consumable, else 0. */
static uint8_t standing_on_consumable(struct Thing * t_standing);



static void get_neighbor_scores(uint16_t * score_map, uint16_t pos_i,
                                uint16_t max_score, uint16_t * neighbors)
{
    uint32_t map_size = world.map.length * world.map.length;
    uint8_t i_dir;
    for (i_dir = 0; i_dir < N_DIRS; neighbors[i_dir] = max_score, i_dir++);
    uint8_t open_north     = pos_i >= world.map.length;
    uint8_t open_east      = pos_i + 1 % world.map.length;
    uint8_t open_south     = pos_i + world.map.length < map_size;
    uint8_t open_west      = pos_i % world.map.length;
    uint8_t is_indented    = (pos_i / world.map.length) % 2;
    uint8_t open_diag_west = is_indented || open_west;
    uint8_t open_diag_east = !is_indented || open_east;
    if (open_north && open_diag_east)
    {
        neighbors[0] = score_map[pos_i - world.map.length + is_indented];
    }
    if (open_east)
    {
        neighbors[1] = score_map[pos_i + 1];
    }
    if (open_south && open_diag_east)
    {
        neighbors[2] = score_map[pos_i + world.map.length + is_indented];
    }
    if (open_south && open_diag_west)
    {
        neighbors[3] = score_map[pos_i + world.map.length - !is_indented];
    }
    if (open_west)
    {
        neighbors[4] = score_map[pos_i - 1];
    }
    if (open_north && open_diag_west)
    {
        neighbors[5] = score_map[pos_i - world.map.length - !is_indented];
    }
}



static void dijkstra_map(uint16_t * score_map, uint16_t max_score)
{
    uint32_t map_size = world.map.length * world.map.length;
    uint32_t pos;
    uint16_t i_scans, neighbors[N_DIRS], min_neighbor;
    uint8_t scores_still_changing = 1;
    uint8_t i_dirs;
    for (i_scans = 0; scores_still_changing; i_scans++)
    {
        scores_still_changing = 0;
        for (pos = 0; pos < map_size; pos++)
        {
            if (score_map[pos] <= max_score)
            {
                get_neighbor_scores(score_map, pos, max_score, neighbors);
                min_neighbor = max_score;
                for (i_dirs = 0; i_dirs < N_DIRS; i_dirs++)
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
}



static void init_score_map(char filter, uint16_t * score_map, uint32_t map_size,
                           struct Thing * t_eye)
{
    uint32_t i;
    for (i = 0; i < map_size; i++)
    {
        score_map[i] = UINT16_MAX;
        if ('.' == t_eye->mem_map[i])
        {
            score_map[i] = UINT16_MAX-1;
        }
    }
    if      ('e' == filter)
    {
        struct Thing * t = world.things;
        for (; t; t = t->next)
        {
            if (   t==t_eye || !t->lifepoints
                || 'H' == t_eye->fov_map[t->pos.y*world.map.length + t->pos.x])
            {
                continue;
            }
            else if (t->lifepoints && t->type == t_eye->type)
            {
                score_map[t->pos.y * world.map.length + t->pos.x] = UINT16_MAX;
                continue;
            }
            score_map[t->pos.y * world.map.length + t->pos.x] = 0;
        }
    }
    else if ('c' == filter)
    {
        struct ThingInMemory * tm = t_eye->t_mem;
        for (; tm; tm = tm->next)
        {
            if (' ' == t_eye->mem_map[tm->pos.y * world.map.length + tm->pos.x])
            {
                continue;
            }
            struct ThingType * tt = get_thing_type(tm->type);
            if (!tt->consumable)
            {
                continue;
            }
            score_map[tm->pos.y * world.map.length + tm->pos.x] = 0;
        }
    }
    else if (('0' < filter && '9' >= filter) || ' ' == filter)
    {
        uint32_t i;
        for (i = 0; i < (uint32_t) (world.map.length * world.map.length); i++)
        {
            score_map[i] = filter == t_eye->mem_depth_map[i] ? 0 : score_map[i];
        }
    }
}



static uint8_t get_dir_to_nearest_target(struct Thing * t_eye, char filter)
{
    char dir_to_nearest_target = 0;
    uint8_t run_i = 9 /* maximum mem depth age below never-explored */ + 1;
    uint8_t mem_depth_char = ' ';
    while (run_i && ('s' == filter || seeing_thing(t_eye, filter)))
    {
        run_i = 's' != filter ? 0 : run_i - 1;
        uint32_t map_size = world.map.length * world.map.length;
        uint16_t * score_map = try_malloc(map_size * sizeof(uint16_t),__func__);
        init_score_map('s' == filter ? mem_depth_char : filter,
                       score_map, map_size, t_eye);
        mem_depth_char = ' ' == mem_depth_char ? '9' : mem_depth_char - 1;
        dijkstra_map(score_map, UINT16_MAX-1);
        uint16_t neighbors[N_DIRS];
        uint16_t pos_i = (t_eye->pos.y * world.map.length) + t_eye->pos.x;
        get_neighbor_scores(score_map, pos_i, UINT16_MAX-1, neighbors);
        free(score_map);
        uint16_t min_neighbor = UINT16_MAX-1;
        char * dirs = "edcxsw";/* get_neighbor_scores()'s clockwise dir order.*/
        uint8_t i;
        for (i = 0; i < N_DIRS; i++)
        {
            if (min_neighbor > neighbors[i])
            {
                min_neighbor = neighbors[i];
                dir_to_nearest_target = dirs[i];
            }
        }
        if (dir_to_nearest_target)
        {
            t_eye->command = get_thing_action_id_by_name(s[S_CMD_MOVE]);
            t_eye->arg = dir_to_nearest_target;
            run_i = 0;
        }
    }
    return dir_to_nearest_target;
}



static uint8_t seeing_thing(struct Thing * t_eye, char filter)
{
    if (t_eye->fov_map && 'e' == filter)
    {
        struct Thing * t = world.things;
        for (; t; t = t->next)
        {
            if (   t != t_eye
                && 'v' == t_eye->fov_map[t->pos.y*world.map.length + t->pos.x])
            {
                if (t->lifepoints && t->type != t_eye->type)
                {
                    return 1;
                }
            }
        }
    }
    else if (t_eye->mem_map && 'c' == filter)
    {
        struct ThingInMemory * tm = t_eye->t_mem;
        for (; tm; tm = tm->next)
        {
            if (' ' != t_eye->mem_map[tm->pos.y * world.map.length + tm->pos.x])
            {
                struct ThingType * tt = get_thing_type(tm->type);
                if (tt->consumable)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}



static int16_t get_inventory_slot_to_consume(struct Thing * t_owner)
{
    uint8_t compare_consumability = 0;
    int16_t selection = -1;
    struct Thing * t = t_owner->owns;;
    uint8_t i;
    for (i = 0; t; t = t->next, i++)
    {
        struct ThingType * tt = get_thing_type(t->type);
        if (tt->consumable > compare_consumability)
        {
            compare_consumability = tt->consumable;
            selection = i;
        }
    }
    return selection;
}



static uint8_t standing_on_consumable(struct Thing * t_standing)
{
    struct Thing * t = world.things;
    for (; t; t = t->next)
    {
        if (   t != t_standing
            && t->pos.y == t_standing->pos.y && t->pos.x == t_standing->pos.x)
        {
            struct ThingType * tt = get_thing_type(t->type);
            if (tt->consumable)
            {
                return 1;
            }
        }
    }
    return 0;
}



extern void ai(struct Thing * t)
{
    t->command = get_thing_action_id_by_name(s[S_CMD_WAIT]);
    if (!get_dir_to_nearest_target(t, 'e'))
    {
        int16_t sel = get_inventory_slot_to_consume(t);
        if (-1 != sel)
        {
            t->command = get_thing_action_id_by_name(s[S_CMD_USE]);
            t->arg = (uint8_t) sel;
        }
        else if (standing_on_consumable(t))
        {
            t->command = get_thing_action_id_by_name(s[S_CMD_PICKUP]);
        }
        else if (!get_dir_to_nearest_target(t, 'c'))
        {
            get_dir_to_nearest_target(t, 's');
        }
    }
}
