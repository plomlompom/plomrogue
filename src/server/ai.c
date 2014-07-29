/* src/server/ai.c */

#include "ai.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <stdlib.h> /* free() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "field_of_view.h" /* VISIBLE */
#include "hardcoded_strings.h" /* s */
#include "thing_actions.h" /* get_thing_action_id_by_name() */
#include "things.h" /* struct Thing */
#include "world.h" /* global world */



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
 * directions. If it's neighbors are low enough that the result would be lower
 * than the current value, re-set it to 1 point higher than its lowest-scored
 * neighbor. Repeat this whole process until all cells have settled on their
 * final score. Ignore cells whose position in "score_map" fits cells of
 * unreachable terrain in world.map.cells or whose score is greater than
 * "max_score". Expect "max_score" to be the maximum score for cells, marking
 * them as unreachable.
 */
static void dijkstra_map(uint16_t * score_map, uint16_t max_score);

/* Return numpad char of direction ("8", "6", "2", "4" etc.) of enemy with the
 * shortest path visible to "t_origin". If no enemy is around, return 0.
 */
static char get_dir_to_nearest_enemy(struct Thing * thing_origin);



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
            if ('.' == world.map.cells[pos] && score_map[pos] <= max_score)
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



static char get_dir_to_nearest_enemy(struct Thing * t_origin)
{
    /* Calculate for each cell distance to visibly nearest enemy, with movement
     * possible in the directions or "dir". (Actor's own cells start with 0
     * distance towards themselves. Cells of actors of own type are invisible.)
     */
    uint32_t map_size = world.map.length * world.map.length;
    uint16_t max_score = UINT16_MAX - 1;
    uint16_t * score_map = try_malloc(map_size * sizeof(uint16_t), __func__);
    uint32_t i;
    for (i = 0; i < map_size; i++)
    {
        score_map[i] = t_origin->fov_map[i] & VISIBLE ? max_score : UINT16_MAX;

    }
    struct Thing * t = world.things;
    for (; t != NULL; t = t->next)
    {
        if (!t->lifepoints || t == t_origin)
        {
            continue;
        }
        if (t->lifepoints && t->type == t_origin->type)
        {
            score_map[t->pos.y * world.map.length + t->pos.x] = UINT16_MAX;
            continue;
        }
        score_map[t->pos.y * world.map.length + t->pos.x] = 0;
    }
    dijkstra_map(score_map, max_score);

    /* Return direction of "t_origin"'s lowest-scored neighbor cell. */
    uint16_t neighbors[N_DIRS];
    uint16_t pos_i = (t_origin->pos.y * world.map.length) + t_origin->pos.x;
    get_neighbor_scores(score_map, pos_i, max_score, neighbors);
    free(score_map);
    char dir_to_nearest_enemy = 0;
    uint16_t min_neighbor = max_score;
    char * dirs = "edcxsw";    /* get_neighbor_scores()'s clockwise dir order.*/
    for (i = 0; i < N_DIRS; i++)
    {
        if (min_neighbor > neighbors[i])
        {
            min_neighbor = neighbors[i];
            dir_to_nearest_enemy = dirs[i];
        }
    }
    return dir_to_nearest_enemy;
}



extern void ai(struct Thing * t)
{
    t->command = get_thing_action_id_by_name(s[S_CMD_WAIT]);
    char sel = t->fov_map ? get_dir_to_nearest_enemy(t) : 0;/* t->fov_map may */
    if (0 != sel)                                           /* be absent due  */
    {                                                       /* to god command.*/
        t->command = get_thing_action_id_by_name(s[S_CMD_MOVE]);
        t->arg = sel;
    }
}
