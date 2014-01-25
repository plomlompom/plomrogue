/* src/server/map_objects.c */

#include "map_objects.h"
#include <stddef.h> /* NULL */
#include <stdio.h> /* FILE typedef */
#include <stdint.h> /* uint8_t, uint16_t */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* strlen(), memcpy(), strtok(), memset() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), try_fgets(),
                                  * textfile_sizes()
                                  */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint16.h" /* yx_uint16 struct */
#include "cleanup.h" /* set_cleanup_flag() */
#include "map.h" /* is_passable() */
#include "rrand.h" /* rrand() */
#include "world.h" /* global world */
#include "yx_uint16.h" /* yx_uint16_cmp() */



/* Return pointer to map object of "id" in chain starting at "ptr". */
static struct MapObj * get_map_object(struct MapObj * ptr, uint8_t id);

/* Return random passable (as by is_passable()) position on world.map. */
static struct yx_uint16 find_passable_pos();

/* Add object of "type" to map on random position. Don't place actor on actor.*/
static void add_map_object(uint8_t type);



static struct MapObj * get_map_object(struct MapObj * ptr, uint8_t id)
{
    while (1)
    {
        if (NULL == ptr || id == ptr->id)
        {
            return ptr;
        }
        struct MapObj * owned_object = get_map_object(ptr->owns, id);
        if (NULL != owned_object)
        {
            return ptr;
        }
        ptr = ptr->next;
    }
}



static struct yx_uint16 find_passable_pos()
{
    struct yx_uint16 pos;
    for (pos.y = pos.x = 0; 0 == is_passable(pos);)
    {
        pos.y = rrand() % world.map.size.y;
        pos.x = rrand() % world.map.size.x;
    }
    return pos;
}



static void add_map_object(uint8_t type)
{
    char * f_name = "add_map_object()";
    struct MapObjDef * mod = get_map_object_def(type);
    struct MapObj *    mo  = try_malloc(sizeof(struct MapObj), f_name);
    memset(mo, 0, sizeof(struct MapObj));
    mo->id         = world.map_obj_count++;
    mo->type       = mod->id;
    mo->lifepoints = mod->lifepoints;
    while (1)
    {
        struct yx_uint16 pos = find_passable_pos(world.map);
        struct MapObj * mo_ptr;
        uint8_t clear = 1;
        for (mo_ptr = world.map_objs; mo_ptr != NULL; mo_ptr = mo_ptr->next)
        {
            if (yx_uint16_cmp(&pos, &mo_ptr->pos) && 0 != mo_ptr->lifepoints)
            {
                clear = 0;
                break;
            }
        }
        if (1 == clear)
        {
            mo->pos = pos;
            break;
        }
    }
    struct MapObj ** mo_ptr_ptr = &world.map_objs;
    for (; NULL != * mo_ptr_ptr; mo_ptr_ptr = &(*mo_ptr_ptr)->next);
    * mo_ptr_ptr = mo;
}



extern void init_map_object_defs(char * filename)
{
    char * f_name = "init_map_object_defs()";
    FILE * file = try_fopen(filename, "r", f_name);
    uint16_t linemax = textfile_sizes(file, NULL);
    struct MapObjDef ** last_mod_ptr_ptr = &world.map_obj_defs;
    char * delim = " ";
    char line[linemax + 1];
    while (try_fgets(line, linemax + 1, file, f_name))
    {
        struct MapObjDef * mod = try_malloc(sizeof(struct MapObjDef), f_name);
        mod->next = NULL;
        mod->id = atoi(strtok(line, delim));
        mod->corpse_id = atoi(strtok(NULL, delim));
        mod->char_on_map = * strtok(NULL, delim);
        mod->lifepoints = atoi(strtok(NULL, delim));
        char * name = strtok(NULL, "\n");
        mod->name = try_malloc(strlen(name) + 1, f_name);
        memcpy(mod->name, name, strlen(name) + 1);
        * last_mod_ptr_ptr = mod;
        last_mod_ptr_ptr = &mod->next;
    }
    try_fclose(file, f_name);
    set_cleanup_flag(CLEANUP_MAP_OBJECT_DEFS);
}



extern void free_map_object_defs(struct MapObjDef * mod_start)
{
    if (NULL == mod_start)
    {
        return;
    }
    free_map_object_defs(mod_start->next);
    free(mod_start->name);
    free(mod_start);
    mod_start = NULL;
}



extern void add_map_objects(uint8_t type, uint8_t n)
{
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        add_map_object(type);
    }
}



extern void free_map_objects(struct MapObj * mo_start)
{
    if (NULL == mo_start)
    {
        return;
    }
    free_map_objects(mo_start->owns);
    free_map_objects(mo_start->next);
    free(mo_start);
    if (mo_start == world.map_objs)
    {
        world.map_objs = NULL;
    }
}



extern void own_map_object(struct MapObj ** target, struct MapObj ** source,
                           uint8_t id)
{
    struct MapObj * mo;
    if (id == (*source)->id)
    {
        mo = * source;
        * source = mo->next;
    }
    else
    {
        struct MapObj * penult = * source;
        while (1)
        {
            if (id == penult->next->id)
            {
                break;
            }
            penult = penult->next;
        }
        mo = penult->next;
        penult->next = mo->next;
    }
    struct MapObj ** mo_ptr_ptr = target;
    for (; NULL != * mo_ptr_ptr; mo_ptr_ptr = &(*mo_ptr_ptr)->next);
    * mo_ptr_ptr = mo;
    mo->next = NULL;
}



extern struct MapObj * get_player()
{
    return get_map_object(world.map_objs, 0);
}



extern struct MapObjDef * get_map_object_def(uint8_t id)
{
    struct MapObjDef * mod = world.map_obj_defs;
    for (; id != mod->id; mod = mod->next);
    return mod;
}



extern void set_object_position(struct MapObj * mo, struct yx_uint16 pos)
{
    mo->pos = pos;
    struct MapObj * owned = mo->owns;
    for (; owned != NULL; set_object_position(owned, pos), owned = owned->next);
}
