/* src/server/map_objects.c */

#include "map_objects.h"
#include <stddef.h> /* NULL */
#include <stdio.h> /* FILE typedef */
#include <stdint.h> /* uint8_t, uint16_t, UINT8_MAX */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* strlen(), memcpy(), memset() */
#include "../common/err_try_fgets.h" /* err_try_fgets(), err_line(),
                                      * reset_err_try_fgets_counter()
                                      */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), try_fgetc(),
                                  * textfile_width()
                                  */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* yx_uint8 struct */
#include "cleanup.h" /* set_cleanup_flag() */
#include "map.h" /* is_passable() */
#include "rrand.h" /* rrand() */
#include "world.h" /* global world */
#include "yx_uint8.h" /* yx_uint8_cmp() */



/* Return pointer to map object of "id" in chain starting at "ptr". */
static struct MapObj * get_map_object(struct MapObj * ptr, uint8_t id);

/* Return random passable (as by is_passable()) position on world.map. */
static struct yx_uint8 find_passable_pos();

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



static struct yx_uint8 find_passable_pos()
{
    struct yx_uint8 pos;
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
        struct yx_uint8 pos = find_passable_pos(world.map);
        struct MapObj * mo_ptr;
        uint8_t clear = 1;
        for (mo_ptr = world.map_objs; mo_ptr != NULL; mo_ptr = mo_ptr->next)
        {
            if (yx_uint8_cmp(&pos, &mo_ptr->pos) && 0 != mo_ptr->lifepoints)
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



extern void init_map_object_defs()
{
    char * f_name = "init_map_object_defs()";
    char * context = "Failed reading map object definitions file. ";
    char * err_toolarge = "Value is too large.";
    char * err_uniq     = "Declaration of ID already used.";
    FILE * file = try_fopen(world.path_map_obj_defs, "r", f_name);
    uint32_t linemax = textfile_width(file);
    struct MapObjDef ** last_mod_ptr_ptr = &world.map_obj_defs;
    char line[linemax + 1];
    reset_err_try_fgets_counter();
    while (1)
    {
        int test_for_end = try_fgetc(file, f_name);
        if (EOF == test_for_end || '\n' == test_for_end)
        {
            break;
        }
        exit_trouble(EOF == ungetc(test_for_end, file), f_name, "ungetc()");
        struct MapObjDef * mod = try_malloc(sizeof(struct MapObjDef), f_name);
        mod->next = NULL;
        err_try_fgets(line, linemax, file, context, "nfi");
        err_line(atoi(line) > UINT8_MAX, line, context, err_toolarge);
        mod->id = atoi(line);
        struct MapObjDef * mod_test = world.map_obj_defs;
        for (; NULL != mod_test; mod_test = mod_test->next)
        {
            err_line(mod->id == mod_test->id, line, context, err_uniq);
        }
        err_try_fgets(line, linemax, file, context, "0nfi");
        err_line(atoi(line) > UINT8_MAX, line, context, err_toolarge);
        mod->corpse_id = atoi(line);
        err_try_fgets(line, linemax, file, context, "0nfs");
        mod->char_on_map = line[0];
        err_try_fgets(line, linemax, file, context, "0nfi");
        err_line(atoi(line) > UINT8_MAX, line, context, err_toolarge);
        mod->lifepoints = atoi(line);
        err_try_fgets(line, linemax, file, context, "0nf");
        line[strlen(line) - 1] = '\0';
        mod->name = try_malloc(strlen(line) + 1, f_name);
        memcpy(mod->name, line, strlen(line) + 1);
        * last_mod_ptr_ptr = mod;
        last_mod_ptr_ptr = &mod->next;
        err_try_fgets(line, linemax, file, context, "d");
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
    if (mo_start == world.map_objs)  /* So add_map_objects()' NULL-delimited  */
    {                                /* map object iteration loop does not    */
        world.map_objs = NULL;       /* iterate over freed memory when called */
    }                                /* the 1st time after world re-seeding.  */
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
    for (; NULL != mod && id != mod->id; mod = mod->next);
    char * err_intro = "Requested map object definition of unused ID ";
    char err[strlen(err_intro) + 3 + 1 + 1];
    sprintf(err, "%s%d.", err_intro, id);
    exit_err(NULL == mod, err);
    return mod;
}



extern void set_object_position(struct MapObj * mo, struct yx_uint8 pos)
{
    mo->pos = pos;
    struct MapObj * owned = mo->owns;
    for (; owned != NULL; set_object_position(owned, pos), owned = owned->next);
}
