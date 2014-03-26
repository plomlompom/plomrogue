/* src/server/ai.c */

#include "ai.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <stdlib.h> /* free() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "map_object_actions.h" /* get_moa_id_by_name() */
#include "map_objects.h" /* struct MapObj */
#include "world.h" /* global world */



#define N_DIRS 8



/* Write into "neighbors" scores of the eight immediate 2D neighbors of the
 * "score_map" cell at "pos_i" (array index), as found in the directions north,
 * north-east, east etc. (clockwise order). "max_score" is used for illegal
 * neighborhoods (i.e. if the direction would lead beyond the map's border).
 */
static void get_neighbor_scores(uint32_t * score_map, uint16_t pos_i,
                                uint32_t max_score, uint32_t * neighbors);

/* Iterate over scored cells in "score_map" of world.map's 2D geometry. Compare
 * each cell's score against the score of its immediate neighbors in N_DIRS
 * directions. If it's neighbors are low enough that the result would be lower
 * than the current value, re-set it to world.map.dist_orthogonal points higher
 * than its lowest-scored orthogonal neighbor or world.map.dist_diagonal points
 * higher than its lowest-scored diagonal neighbor (whatever would result in a
 * lower value). Repeat this whole process until all cells have settled on their
 * final score. Ignore cells whose position in "score_map" fits cells of
 * unreachable terrain in world.map.cells. Expect "max_score" to be the maximum
 * score for cells, marking them as unreachable.
 */
static void dijkstra_map(uint32_t * score_map, uint32_t max_score);

/* Return numpad char of direction ("8", "6", "2", "4" etc.) of enemy with the
 * shortest path to "mo_origin". If no enemy is around, return 0.
 */
static char get_dir_to_nearest_enemy(struct MapObj * mo_origin);



static void get_neighbor_scores(uint32_t * score_map, uint16_t pos_i,
                                uint32_t max_score, uint32_t * neighbors)
{
    uint32_t map_size = world.map.size.y * world.map.size.x;
    uint8_t i_dir;
    for (i_dir = 0; i_dir < N_DIRS; neighbors[i_dir] = max_score, i_dir++);
    uint8_t open_north = pos_i >= world.map.size.x;
    uint8_t open_east  = pos_i + 1 % world.map.size.x;
    uint8_t open_south = pos_i + world.map.size.x < map_size;
    uint8_t open_west  = pos_i % world.map.size.x;
    if (open_north)
    {
        neighbors[0] = score_map[pos_i - world.map.size.x];
    }
    if (open_north && open_east)
    {
        neighbors[1] = score_map[pos_i - world.map.size.x + 1];
    }
    if (open_east)
    {
        neighbors[2] = score_map[pos_i + 1];
    }
    if (open_east && open_south)
    {
        neighbors[3] = score_map[pos_i + 1 + world.map.size.x];
    }
    if (open_south)
    {
        neighbors[4] = score_map[pos_i + world.map.size.x];
    }
    if (open_south && open_west)
    {
        neighbors[5] = score_map[pos_i + world.map.size.x - 1];
    }
    if (open_west)
    {
        neighbors[6] = score_map[pos_i - 1];
    }
    if (open_west && open_north)
    {
        neighbors[7] = score_map[pos_i - 1 - world.map.size.x];
    }
}



static void dijkstra_map(uint32_t * score_map, uint32_t max_score)
{
    uint32_t i_scans, neighbors[N_DIRS], min_neighbor_o, min_neighbor_d;
    uint32_t map_size = world.map.size.y * world.map.size.x;
    uint16_t pos;
    uint8_t scores_still_changing = 1;
    uint8_t i_dirs;
    for (i_scans = 0; scores_still_changing; i_scans++)
    {
        scores_still_changing = 0;
        for (pos = 0; pos < map_size; pos++)
        {
            if ('.' == world.map.cells[pos])
            {
                get_neighbor_scores(score_map, pos, max_score, neighbors);
                min_neighbor_d = max_score;
                min_neighbor_o = max_score;
                for (i_dirs = 0; i_dirs < N_DIRS; i_dirs++)
                {
                    if   (!(i_dirs % 2) && min_neighbor_o > neighbors[i_dirs])
                    {
                        min_neighbor_o = neighbors[i_dirs];
                    }
                    else if (i_dirs % 2 && min_neighbor_d > neighbors[i_dirs])
                    {
                        min_neighbor_d = neighbors[i_dirs];
                    }
                }
                if (score_map[pos] > min_neighbor_o + world.map.dist_orthogonal)
                {
                    score_map[pos] = min_neighbor_o + world.map.dist_orthogonal;
                    scores_still_changing = 1;
                }
                if (score_map[pos] > min_neighbor_d + world.map.dist_diagonal)
                {
                    score_map[pos] = min_neighbor_d + world.map.dist_diagonal;
                    scores_still_changing = 1;
                }
            }
        }
    }
}



static char get_dir_to_nearest_enemy(struct MapObj * mo_origin)
{
    char * f_name = "get_dir_to_nearest_enemy()";

    /* Calculate for each cell the distance to the nearest map actor that is
     * not "mo_origin", with movement only possible in the directions of "dir".
     * (Actors' own cells start with a distance of 0 towards themselves.)
     */
    uint32_t map_size = world.map.size.y * world.map.size.x;
    uint32_t max_score = UINT32_MAX - (world.map.dist_diagonal + 1);
    uint32_t * score_map = try_malloc(map_size * sizeof(uint32_t), f_name);
    uint32_t i;
    for (i = 0; i < map_size; i++)
    {
        score_map[i] = max_score;
    }
    struct MapObj * mo = world.map_objs;
    for (; mo != NULL; mo = mo->next)
    {
        if (!mo->lifepoints || mo == mo_origin)
        {
            continue;
        }
        score_map[(mo->pos.y * world.map.size.x) + mo->pos.x] = 0;
    }
    dijkstra_map(score_map, max_score);

    /* Return direction of "mo_origin"'s lowest-scored neighbor cell. */
    uint32_t neighbors[N_DIRS];
    uint16_t pos_i = (mo_origin->pos.y * world.map.size.x) + mo_origin->pos.x;
    get_neighbor_scores(score_map, pos_i, max_score, neighbors);
    free(score_map);
    char dir_to_nearest_enemy = 0;
    uint32_t min_neighbor = max_score;
    char * dirs = "89632147";  /* get_neighbor_scores()'s clockwise dir order.*/
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



extern void ai(struct MapObj * mo)
{
    mo->command = get_moa_id_by_name("wait");
    char sel = get_dir_to_nearest_enemy(mo);
    if (0 != sel)
    {
        mo->command = get_moa_id_by_name("move");
        mo->arg = sel;
    }
}
