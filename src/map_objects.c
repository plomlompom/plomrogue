/* map_objects.c */

#include "map_objects.h"
#include <stdlib.h> /* for malloc(), calloc(), free(), atoi() */
#include <stdint.h> /* for uint8_t */
#include <stdio.h> /* for FILE typedef */
#include <string.h> /* for strchr(), strlen(), memcpy(), strtok() */
#include "readwrite.h" /* for [read/write]_uint[8/16/23][_bigendian]() */
#include "misc.h" /* for textfile_sizes(), find_passable_pos() */
#include "main.h" /* for World struct */
#include "rexit.h" /* for err_exit() */



/* Return pointer to newly allocated map object struct of size "size". If first
 * in map object chain ("first" pointing to !0), point "start" to it.
 *
 * Returns NULL instead of MapObj pointer if malloc() failed.
 */
static struct MapObj * get_next_map_obj(void * start, char * first,
                                        size_t size, struct MapObj * map_obj);



/* Map-object-type-specific helpers to (build|write|read)_map_objects(). */
static void build_map_objects_itemdata(struct MapObjDef * map_obj_def,
                                       void * start);
static void build_map_objects_monsterdata(struct MapObjDef * map_obj_def,
                                          void * start);
static uint8_t write_map_objects_monsterdata(void * start, FILE * file);
static uint8_t read_map_objects_monsterdata( void * start, FILE * file);



static struct MapObj * get_next_map_obj(void * start, char * first,
                                        size_t size, struct MapObj * map_obj)
{
    if (* first)
    {
        struct MapObj * * z = start;
        map_obj = malloc(size);
        * z = map_obj;
        * first = 0;
    }
    else
    {
        map_obj->next = malloc(size);
        map_obj = map_obj->next;
    }
    return map_obj;
}



static void build_map_objects_itemdata(struct MapObjDef * map_obj_def,
                                       void * start)
{
  struct Item * i = (struct Item *) start;
  i->map_obj.type = map_obj_def->id;
}



static void build_map_objects_monsterdata(struct MapObjDef * map_obj_def,
                                          void * start)
{
    struct Monster * m = (struct Monster *) start;
    m->map_obj.type = map_obj_def->id;
    struct MonsterDef * md = (struct MonsterDef *) map_obj_def;
    m->hitpoints = md->hitpoints_start;
}



static uint8_t write_map_objects_monsterdata(void * start, FILE * file)
{
    struct Monster * m = (struct Monster *) start;
    return write_uint8(m->hitpoints, file);
}



static uint8_t read_map_objects_monsterdata (void * start, FILE * file)
{
    struct Monster * m = (struct Monster *) start;
    return read_uint8(file, &m->hitpoints);
}



extern void init_map_object_defs(struct World * world, char * filename)
{
    char * err_o = "Trouble in init_map_object_defs() with fopen().";
    char * err_s = "Trouble in init_map_object_defs() with textfile_sizes().";
    char * err_m = "Trouble in init_map_object_defs() with malloc()/calloc().";
    char * err_c = "Trouble in init_map_object_defs() with fclose().";

    FILE * file = fopen(filename, "r");
    exit_err(NULL == file, world, err_o);
    uint16_t linemax;
    exit_err(textfile_sizes(file, &linemax, NULL), world, err_s);

    struct MapObjDef  mod;
    struct ItemDef    id;
    struct MonsterDef md;
    world->item_def    = 0;
    world->monster_def = 0;
    struct ItemDef    * * p_p_id  = &world->item_def;
    struct MonsterDef * * p_p_md  = &world->monster_def;
    char * defline = malloc(linemax);
    exit_err(NULL == defline, world, err_m);
    char * line_p;
    char * delim = " ";
    while (fgets(defline, linemax, file))
    {
        mod.next    = 0;
        mod.id      = atoi(strtok(defline, delim));
        mod.m_or_i  = * strtok(NULL, delim);
        mod.mapchar = * strtok(NULL, delim);
        if ('i' == mod.m_or_i)
        {
            line_p = strtok(NULL, delim);
        }
        else
        {
            md.corpse_id       = atoi(strtok(NULL, delim));
            md.hitpoints_start = atoi(strtok(NULL, delim));
            line_p             = strtok(NULL, delim);
        }
        mod.desc = calloc(strlen(line_p), sizeof(char));
        exit_err(NULL == mod.desc, world, err_m);
        memcpy(mod.desc, line_p, strlen(line_p) - 1);
        if ('i' == mod.m_or_i)
        {
            id.map_obj_def = mod;
            * p_p_id       = malloc(sizeof(struct ItemDef));
            exit_err(NULL == p_p_id, world, err_m);
            * * p_p_id     = id;
            p_p_id         = (struct ItemDef    * *) * p_p_id;
        }
        else
        {
            md.map_obj_def = mod;
            * p_p_md       = malloc(sizeof(struct MonsterDef));
            exit_err(NULL == p_p_md, world, err_m);
            * * p_p_md     = md;
            p_p_md         = (struct MonsterDef * *) * p_p_md;
        }
    }

    free(defline);
    exit_err(fclose(file), world, err_c);
}



extern void free_item_defs(struct ItemDef * id_start)
{
    if (0 != id_start->map_obj_def.next)
    {
        free_item_defs((struct ItemDef *) id_start->map_obj_def.next);
    }
    free(id_start->map_obj_def.desc);
    free(id_start);
}




extern void free_monster_defs(struct MonsterDef * md_start)
{
    if (0 != md_start->map_obj_def.next)
    {
        free_monster_defs((struct MonsterDef *) md_start->map_obj_def.next);
    }
    free(md_start->map_obj_def.desc);
    free(md_start);
}



extern uint8_t write_map_objects(struct World * world, void * start,
                                 FILE * file)
{
    struct MapObj * map_obj;
    struct MapObjDef * mod;
    for (map_obj = start; map_obj != 0; map_obj = map_obj->next)
    {
        if (   write_uint8(map_obj->type, file)
            || write_uint16_bigendian(map_obj->pos.y + 1, file)
            || write_uint16_bigendian(map_obj->pos.x + 1, file))
        {
            return 1;
        }
        mod = get_map_obj_def(world, map_obj->type);
        if ('m' == mod->m_or_i)
        {
            if (write_map_objects_monsterdata(map_obj, file))
            {
                return 1;
            }
        }
    }
    return write_uint16_bigendian(0, file);
}



extern uint8_t read_map_objects(struct World * world, void * start, FILE * file)
{
    char * err = "Trouble in read_map_objects() with get_next_map_obj().";
    struct MapObj * map_obj;
    struct MapObjDef * mod;
    size_t size;
    uint8_t type;
    char first = 1;
    long pos;
    uint16_t read_uint16 = 0;
    while (1)
    {
        pos = ftell(file);
        if (read_uint16_bigendian(file, &read_uint16))
        {
            return 1;
        }
        if (0 == read_uint16)
        {
            break;
        }
        fseek(file, pos, SEEK_SET);
        if (read_uint8(file, &type))
        {
            return 1;
        }
        mod = get_map_obj_def(world, type);
        if ('m' == mod->m_or_i)
        {
            size = sizeof(struct Monster);
        }
        else
        {
            size = sizeof(struct Item);
        }
        map_obj = get_next_map_obj(start, &first, size, map_obj);
        exit_err(NULL == map_obj, world, err);
        map_obj->type = type;
        if (   read_uint16_bigendian(file, &map_obj->pos.y)
            || read_uint16_bigendian(file, &map_obj->pos.x))
        {
            return 1;
        }
        map_obj->pos.y--;
        map_obj->pos.x--;
        if ('m' == mod->m_or_i)
        {
            if (read_map_objects_monsterdata(map_obj, file))
            {
                return 1;
            }
        }
    }
    if (!first)
    {
        map_obj->next = 0;
    }
    return 0;
}



extern void * build_map_objects(struct World * world, void * start, char def_id,
                                uint8_t n)
{
    char * err = "Trouble in build_map_objects() with get_next_map_obj().";
    uint8_t i;
    struct MapObj * mo;
    char first = 1;
    struct MapObjDef * mod = get_map_obj_def(world, def_id);
    size_t size = 0;
    if ('i' == mod->m_or_i)
    {
        size = sizeof(struct Item);
    }
    else
    {
        size = sizeof(struct Monster);
    }
    for (i = 0; i < n; i++)
    {
        mo = get_next_map_obj(start, &first, size, mo);
        exit_err(NULL == mo, world, err);
        mo->pos = find_passable_pos(world->map);
        if ('i' == mod->m_or_i)
        {
            build_map_objects_itemdata(mod, mo);
        }
        else
        {
            build_map_objects_monsterdata(mod, mo);
        }

    }
    if (!first)
    {
        mo->next = 0;
    }
    return &mo->next;
}



extern void free_items(struct Item * item)
{
    if (0 == item)
    {
        return;
    }
    free_items((struct Item *) item->map_obj.next);
    free(item);
}



extern void free_monsters(struct Monster * monster)
{
    if (0 == monster)
    {
        return;
    }
    free_monsters((struct Monster *) monster->map_obj.next);
    free(monster);
}



extern struct MapObjDef * get_map_obj_def(struct World * world, char def_id)
{
    struct MapObjDef * d = NULL;
    for (d = (struct MapObjDef *) world->monster_def;
         d->id != def_id && 0 != d->next;
         d = d->next);
    if (d->id != def_id)
    {
        for (d = (struct MapObjDef *) world->item_def;
             d->id != def_id && 0 != d->next;
             d = d->next);
    }
    return d;
}
