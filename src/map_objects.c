/* map_objects.c */

#include "map_objects.h"
#include <stdlib.h> /* for free(), atoi() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include <stdio.h> /* for FILE typedef */
#include <string.h> /* for strlen(), memcpy(), strtok() */
#include "readwrite.h" /* for textfile_sizes(), try_fopen(), try_fclose(),
                        * try_fgets()
                        */
#include "misc.h" /* for try_malloc(), find_passable_pos() */
#include "main.h" /* for world global */
#include "rexit.h" /* for exit_err() */
#include "yx_uint16.h" /* for yx_uint16 struct, yx_uint16_cmp() */



/* Write representation of "mo" and all of the map objects it owns to "file". */
static void write_map_object(FILE * file, struct MapObj * mo);

/* Return pointer to map object of "id" in chain starting at "ptr". */
static struct MapObj * get_map_object(struct MapObj * ptr, uint8_t id);



static void write_map_object(FILE * file, struct MapObj * mo)
{
    char * f_name = "write_map_object()";
    struct MapObj * mo_ptr = mo->owns;
    uint8_t i = 0;
    for (; NULL != mo_ptr; mo_ptr = mo_ptr->next, i++);
    uint8_t size = 3+1 + 3+1 + 3+1 + 5+1 + 5 + ((1+3)*i) + 1 + 1;
    char line[size];
    sprintf(line, "%d %d %d %d %d %d %d %d",
                  mo->id, mo->type, mo->lifepoints, mo->pos.y, mo->pos.x,
                  mo->command, mo->arg, mo->progress);
    for (mo_ptr = mo->owns; NULL != mo_ptr; mo_ptr = mo_ptr->next)
    {
        sprintf(line + strlen(line), " %d", mo_ptr->id);
    }
    line[strlen(line) + 1] = '\0';
    line[strlen(line)] = '\n';
    try_fwrite(line, strlen(line), 1, file, f_name);
    for (mo_ptr = mo->owns; NULL != mo_ptr; mo_ptr = mo_ptr->next)
    {
        write_map_object(file, mo_ptr);
    }
}



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



extern void write_map_objects(FILE * file)
{
    struct MapObj * mo = world.map_objs;
    while (NULL != mo)
    {
        write_map_object(file, mo);
        mo = mo->next;
    }
}



extern void read_map_objects(FILE * file, char * line, int linemax)
{
    char * f_name = "read_map_objects()";
    struct MapObj ** mo_ptr_ptr = &world.map_objs;
    char * delim = " ";
    struct MapObj * mo;
    fpos_t pos;
    exit_err(-1 == fgetpos(file, &pos), f_name);
    while (try_fgets(line, linemax + 1, file, f_name))
    {
        mo = try_malloc(sizeof(struct MapObj), f_name);
        mo->next       = NULL;
        mo->id         = atoi(strtok(line, delim));
        mo->type       = atoi(strtok(NULL, delim));
        mo->lifepoints = atoi(strtok(NULL, delim));
        mo->pos.y      = atoi(strtok(NULL, delim));
        mo->pos.x      = atoi(strtok(NULL, delim));
        mo->command    = atoi(strtok(NULL, delim));;
        mo->arg        = atoi(strtok(NULL, delim));;
        mo->progress   = atoi(strtok(NULL, delim));;
        mo->owns       = NULL;
        * mo_ptr_ptr = mo;
        mo_ptr_ptr = &mo->next;
    }
    exit_err(-1 == fsetpos(file, &pos), f_name);
    while (try_fgets(line, linemax + 1, file, f_name))
    {
        uint8_t id = atoi(strtok(line, delim));
        uint8_t i;
        for (i = 0; i < 7; i++, strtok(NULL, delim));
        char * owned = strtok(NULL, "\n");
        if (NULL != owned)
        {
            mo = get_map_object(world.map_objs, id);
            char * owned_id = "";
            owned_id = strtok(owned, delim);
            while (NULL != owned_id)
            {
                own_map_object(&mo->owns, &world.map_objs, atoi(owned_id));
                owned_id = strtok(NULL, delim);
            }
        }
    }
}



extern void add_map_object(uint8_t type)
{
    char * f_name = "add_map_object()";
    struct MapObjDef * mod = get_map_object_def(type);
    struct MapObj *    mo  = try_malloc(sizeof(struct MapObj), f_name);
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
    mo->progress = 0;
    mo->command  = 0;
    mo->arg      = 0;
    mo->owns     = NULL;
    mo->next     = NULL;
    struct MapObj ** mo_ptr_ptr = &world.map_objs;
    for (; NULL != * mo_ptr_ptr; mo_ptr_ptr = &(*mo_ptr_ptr)->next);
    * mo_ptr_ptr = mo;
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

