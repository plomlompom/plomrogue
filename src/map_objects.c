/* map_objects.c */

#include "map_objects.h"
#include <stdlib.h> /* for free(), atoi() */
#include <stdint.h> /* for uint8_t */
#include <stdio.h> /* for FILE typedef */
#include <string.h> /* for strchr(), strlen(), memcpy(), strtok() */
#include "readwrite.h" /* for get_linemax(), try_fopen(), try_fclose()
                        * [read/write]_uint[8/16/23][_bigendian]()
                        */
#include "misc.h" /* for try_malloc(), try_calloc(), find_passable_pos() */
#include "main.h" /* for World struct */
#include "rexit.h" /* for err_exit() */
#include "yx_uint16.h" /* for yx_uint16 struct, yx_uint16_cmp() */



extern void init_map_object_defs(struct World * world, char * filename)
{
    char * f_name = "init_map_object_defs()";
    FILE * file = try_fopen(filename, "r", world, f_name);
    uint16_t linemax = get_linemax(file, world, f_name);
    struct MapObjDef ** last_mod_ptr_ptr = &world->map_obj_defs;
    char * delim = " ";
    char line[linemax + 1];
    while (try_fgets(line, linemax + 1, file, world, f_name))
    {
        struct MapObjDef * mod;
        mod = try_malloc(sizeof(struct MapObjDef), world, f_name);
        mod->next = NULL;
        mod->id = atoi(strtok(line, delim));
        mod->corpse_id = atoi(strtok(NULL, delim));
        mod->char_on_map = * strtok(NULL, delim);
        mod->lifepoints = atoi(strtok(NULL, delim));
        char * name = strtok(NULL, "\n");
        mod->name = try_malloc(strlen(name) + 1, world, f_name);
        memcpy(mod->name, name, strlen(name) + 1);
        * last_mod_ptr_ptr = mod;
        last_mod_ptr_ptr = &mod->next;
    }
    try_fclose(file, world, f_name);
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



extern void write_map_objects(struct World * world, FILE * file)
{
    char * f_name = "write_map_objects()";
    struct MapObj * mo = world->map_objs;
    uint8_t size = 3 + 1 + 3 + 1 + 3 + 1 + 5 + 1 + 5 + 1;
    char line[size];
    while (NULL != mo)
    {
        sprintf(line, "%d %d %d %d %d\n",
                mo->id, mo->type, mo->lifepoints, mo->pos.y, mo->pos.x);
        try_fwrite(line, strlen(line), 1, file, world, f_name);
        mo = mo->next;
    }
}



extern void read_map_objects(struct World * world, FILE * file, char * line,
                              int linemax)
{
    char * f_name = "read_map_objects()";
    struct MapObj ** mo_ptr_ptr = &world->map_objs;
    char * delim = " ";
    struct MapObj * mo;
    while (try_fgets(line, linemax + 1, file, world, f_name))
    {
        mo = malloc(sizeof(struct MapObj));
        mo->next = NULL;
        mo->id = atoi(strtok(line, delim));
        if (mo->id > world->map_obj_count)
        {
            world->map_obj_count = mo->id;
        }
        mo->type = atoi(strtok(NULL, delim));
        mo->lifepoints = atoi(strtok(NULL, delim));
        mo->pos.y = atoi(strtok(NULL, delim));
        mo->pos.x = atoi(strtok(NULL, delim));
        * mo_ptr_ptr = mo;
        mo_ptr_ptr = &mo->next;
    }
}



extern void add_map_object(struct World * world, uint8_t type)
{
    char * f_name = "add_map_object";
    struct MapObjDef * mod = get_map_object_def(world, type);
    struct MapObj * mo = try_malloc(sizeof(struct MapObj), world, f_name);
    mo->id = world->map_obj_count;
    world->map_obj_count++;
    mo->type = mod->id;
    mo->lifepoints = mod->lifepoints;
    while (1)
    {
        struct yx_uint16 pos = find_passable_pos(world->map);
        struct MapObj * mo_ptr;
        uint8_t clear = 1;
        for (mo_ptr = world->map_objs;
             mo_ptr != NULL;
             mo_ptr = mo_ptr->next)
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
    mo->next = NULL;
    struct MapObj ** last_ptr_ptr = &world->map_objs;
    struct MapObj * mo_ptr;
    while (NULL != * last_ptr_ptr)
    {
        mo_ptr = * last_ptr_ptr;
        last_ptr_ptr = & mo_ptr->next;
    }
    * last_ptr_ptr = mo;
}



extern void add_map_objects(struct World * world, uint8_t type, uint8_t n)
{
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        add_map_object(world, type);
    }
}



extern void free_map_objects(struct MapObj * mo_start)
{
    if (NULL == mo_start)
    {
        return;
    }
    free_map_objects(mo_start->next);
    free(mo_start);
}



extern struct MapObj * get_player(struct World * world)
{
    struct MapObj * ptr = world->map_objs;
    while (1)
    {
        if (NULL == ptr)
        {
            return ptr;
        }
        if (0 == ptr->id)
        {
            return ptr;
        }
        ptr = ptr->next;
    }
}



extern struct MapObjDef * get_map_object_def(struct World * w, uint8_t id)
{
    struct MapObjDef * mod = w->map_obj_defs;
    while (id != mod->id)
    {
        mod = mod->next;
    }
    return mod;
}
