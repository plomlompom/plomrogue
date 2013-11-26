/* ai.c */

#include "ai.h"
#include <stdlib.h> /* for free() */
#include <string.h> /* for strlen(), memset() */
#include <stdint.h> /* for uint8_t */
#include "main.h" /* for world global */
#include "misc.h" /* for try_malloc() */
#include "map.h" /* for Map struct */
#include "yx_uint16.h" /* for yx_uint16 struct,yx_uint16_cmp(),mv_yx_in_dir() */
#include "map_objects.h" /* for MapObj struct */
#include "map_object_actions.h" /* for get_moa_id_by_name() */



/* Change cardinal direction string ("NNE" etc.) of any length >1 pointed to by
 * "path_ptr" one step clockwise ("NNE" -> "NEE", "NEE" -> "EEE" etc.).
 */
static void clockwise_path(char ** path_ptr);

/* Return dir char (north = "N" etc.) to enemy nearest to "origin" (beeline). */
static char nearest_enemy_dir(struct yx_uint16 origin);



static void clockwise_path(char ** path_ptr)
{
    char * path = * path_ptr;
    char old_char = path[0];
    char new_char = 'N';
    if      ('N' == old_char)
    {
        new_char = 'E';
    }
    else if ('E' == old_char)
    {
        new_char = 'S';
    }
    else if ('S' == old_char)
    {
        new_char = 'W';
    }
    uint8_t len = strlen(path);
    uint8_t i;
    for (; i < len; i++)
    {
        uint8_t next_i = i + 1;
        if (   next_i == len
            || old_char != path[next_i])
        {
            break;
        }
    }
    path[i] = new_char;
}



static char nearest_enemy_dir(struct yx_uint16 origin)
{
    char * f_name = "nearest_enemy_dir()";
    struct MapObj * mo;
    char sel = 0;
    uint16_t dist_max = world.map->size.y;
    if (world.map->size.x > world.map->size.y)
    {
        dist_max = world.map->size.x;
    }
    uint8_t escape = 0;
    uint8_t dist, i, j;
    for (dist = 1; !escape && dist <= dist_max; dist++)
    {
        char * path = try_malloc(dist + 1, f_name);
        memset(path, 'N', dist);
        path[dist] = '\0';
        for (i = 0; !escape && i < (dist * 4); i++)
        {
            clockwise_path(&path);
            struct yx_uint16 testpos = origin;
            for (j = 0; j < dist; j++)
            {
                testpos = mv_yx_in_dir(path[j], testpos);
            }
            if (yx_uint16_cmp(&testpos, &origin) ||
                testpos.y > world.map->size.y || testpos.x > world.map->size.x)
            {
                continue;
            }
            for (mo = world.map_objs; mo != 0; mo = mo->next)
            {
                if (mo->lifepoints && 1 == yx_uint16_cmp(&testpos, &mo->pos))
                {
                    sel = path[0];
                    escape = 1;
                    break;
                }
            }
        }
        free(path);
    }
    return sel;
}



extern void pretty_dumb_ai(struct MapObj * mo)
{
    mo->command = get_moa_id_by_name("wait");
    char sel = nearest_enemy_dir(mo->pos);
    if (0 != sel)
    {
        mo->command = get_moa_id_by_name("move");
        mo->arg = sel;
    }
}
