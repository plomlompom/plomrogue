/* src/server/ai.c */

#include "ai.h"
#include <stdint.h> /* uint8_t, uint32_t, UINT8_MAX */
#include <string.h> /* strlen(), memset() */
#include "../common/yx_uint16.h" /* struct yx_uint16 */
#include "map_object_actions.h" /* get_moa_id_by_name() */
#include "map_objects.h" /* struct MapObj */
#include "world.h" /* global world */



/* Write into "neighbor_scores" scores for immediate neighbors to cell at
 * "pos_yx" (YX coordinates) and "pos_i" (arry_index) in "score_map". Directions
 * determining neighborhood are defined by the letters of "dir"; their order
 * also determines in what order scores are written into "neighbor_score".
 * "len_dirs" is to store the result of a previous strlen(dir) (so it does not
 * have to be called repeatedly and costly in dijkstra_map(); same reason for
 * "pos_i"'s redundancy.). "max_score" is written into "neighbor_scores" for
 * illegal directions (that from "pos_yx" would lead beyond the map's border).
 */
static void get_neighbor_scores(char * dirs, uint8_t len_dirs,
                                uint8_t * score_map, struct yx_uint16 pos_yx,
                                uint32_t pos_i, uint8_t max_score,
                                uint8_t * neighbor_scores);

/* Iterate over scored cells in "score_map" of world.map's 2D geometry. Compare
 * each cell's score against the scores of its immediate neighbors in "dirs"
 * directions; if at least one of these is lower, re-set the current cell's
 * score to one higher than its lowest neighbor score. Repeat this whole process
 * until all cells have settled on their final score. Ignore cells whose
 * position in "score_map" fits a non-island cell in world.map.cells. Expect
 * "max_score" to be the maximum score for cells, marking them as unreachable.
 */
static void dijkstra_map(char * dirs, uint8_t * score_map, uint8_t max_score);

/* Return char of direction ("N", "E", "S" or "W") of enemy with the shortest
 * path to "mo_target". If no enemy is around, return 0.
 */
static char get_dir_to_nearest_enemy(struct MapObj * mo_target);



static void get_neighbor_scores(char * dirs, uint8_t len_dirs,
                                uint8_t * score_map, struct yx_uint16 pos_yx,
                                uint32_t pos_i, uint8_t max_score,
                                uint8_t * neighbor_scores)
{
    memset(neighbor_scores, max_score, len_dirs);
    uint8_t i_dirs;
    for (i_dirs = 0; i_dirs < len_dirs; i_dirs++)
    {
        if      ('N' == dirs[i_dirs] && pos_yx.y > 0)
        {
            neighbor_scores[i_dirs] = score_map[pos_i - world.map.size.x];
        }
        else if ('E' == dirs[i_dirs] && pos_yx.x < world.map.size.x - 1)
        {
            neighbor_scores[i_dirs] = score_map[pos_i + 1];
        }
        else if ('S' == dirs[i_dirs] && pos_yx.y < world.map.size.y - 1)
        {
            neighbor_scores[i_dirs] = score_map[pos_i + world.map.size.x];
        }
        else if ('W' == dirs[i_dirs] && pos_yx.x > 0)
        {
            neighbor_scores[i_dirs] = score_map[pos_i - 1];
        }
    }
}



static void dijkstra_map(char * dirs, uint8_t * score_map, uint8_t max_score)
{
    uint8_t len_dirs = strlen(dirs);
    uint8_t neighbor_scores[len_dirs];
    struct yx_uint16 pos_yx;
    uint32_t pos_i;
    uint8_t i_scans, i_dirs, local_score, min_neighbor_score;
    uint8_t scores_still_changing = 1;
    for (i_scans = 0; scores_still_changing; i_scans++)
    {
        scores_still_changing = 0;
        for (pos_yx.y = 0, pos_i = 0; pos_yx.y < world.map.size.y; pos_yx.y++)
        {
            for (pos_yx.x = 0; pos_yx.x < world.map.size.x; pos_yx.x++, pos_i++)
            {
                if ('.' == world.map.cells[pos_i])
                {
                    local_score = score_map[pos_i];
                    get_neighbor_scores(dirs, len_dirs, score_map, pos_yx,
                                        pos_i, max_score, neighbor_scores);
                    min_neighbor_score = max_score;
                    for (i_dirs = 0; i_dirs < len_dirs; i_dirs++)
                    {
                        if (min_neighbor_score > neighbor_scores[i_dirs])
                        {
                            min_neighbor_score = neighbor_scores[i_dirs];
                        }
                    }
                    if (local_score > min_neighbor_score + 1)
                    {
                        score_map[pos_i] = min_neighbor_score + 1;
                        scores_still_changing = 1;
                    }
                }
            }
        }
    }
}



static char get_dir_to_nearest_enemy(struct MapObj * mo_target)
{
    /* Calculate for each cell the distance to the nearest map actor that is
     * not "mo_target", with movement only possible in the directions of "dir".
     * (Actors' own cells start with a distance of 0 towards themselves.)
     */
    uint8_t max_score = UINT8_MAX; /* Score for cells treated as unreachable. */
    char * dirs = "NESW";
    uint8_t score_map[world.map.size.y * world.map.size.x];
    memset(score_map, max_score, world.map.size.y * world.map.size.x);
    struct MapObj * mo = world.map_objs;
    for (; mo != NULL; mo = mo->next)
    {
        if (!mo->lifepoints || mo == mo_target)
        {
            continue;
        }
        score_map[(mo->pos.y * world.map.size.x) + mo->pos.x] = 0;
    }
    dijkstra_map(dirs, score_map, max_score);

    /* Return direction of "mo_target"'s lowest-scored neighbor cell. */
    uint8_t len_dirs = strlen(dirs);
    uint32_t pos_i = (mo_target->pos.y * world.map.size.x) + mo_target->pos.x;
    uint8_t neighbor_scores[len_dirs];
    get_neighbor_scores(dirs, len_dirs, score_map, mo_target->pos, pos_i,
                        max_score, neighbor_scores);
    char dir_to_nearest_enemy = 0;
    uint8_t min_neighbor_score = max_score;
    uint8_t i_dirs;
    for (i_dirs = 0; i_dirs < len_dirs; i_dirs++)
    {
        if (min_neighbor_score > neighbor_scores[i_dirs])
        {
            min_neighbor_score = neighbor_scores[i_dirs];
            dir_to_nearest_enemy = dirs[i_dirs];
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
