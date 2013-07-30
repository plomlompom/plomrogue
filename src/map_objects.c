/* map_objects.c */

#include "map_objects.h"
#include <stdlib.h> /* for malloc(), calloc(), free(), atoi() */
#include <stdio.h> /* for FILE typedef */
#include <string.h> /* for strchr(), strlen(), memcpy()  */
#include "readwrite.h" /* for all the map object (def) loading/saving */
#include "misc.h" /* for textfile_sizes(), find_passable_pos() */
#include "main.h" /* for World struct */



/* Return pointer to newly allocated map object struct of size "size". If first
 * in map object chain ("first" pointing to !0), point "start" to it.
 */
static struct MapObj * get_next_map_obj(void * start, char * first,
                                        size_t size, struct MapObj * map_obj);



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



extern void init_map_object_defs(struct World * world, char * filename)
{
    world->item_def = 0;
    world->monster_def = 0;
    FILE *                file    = fopen(filename, "r");
    uint16_t              linemax;
    textfile_sizes (file, &linemax, NULL);
    struct MapObjDef      mod;
    struct ItemDef        id;
    struct MonsterDef     md;
    struct ItemDef    * * p_p_id  = &world->item_def;
    struct MonsterDef * * p_p_md  = &world->monster_def;
    char *                defline = malloc(linemax);
    char *                line_p;
    char                  m_or_i;
    while (fgets(defline, linemax, file))
    {
        mod.next    = 0;
        mod.id      = atoi(defline);
        line_p      = strchr(defline, ' ') + 1;
        m_or_i      = * line_p;
        mod.mapchar = * (line_p + 2);
        if ('i' == m_or_i)
        {
            line_p = line_p + 5;
        }
        else
        {
            md.hitpoints_start = atoi   (line_p + 4);
            line_p             = strchr (line_p + 4, ' ') + 1;
        }
        mod.desc = calloc (strlen (line_p), sizeof(char));
        memcpy (mod.desc, line_p, strlen(line_p) - 1);
        if ('i' == m_or_i)
        {
            id.map_obj_def = mod;
            * p_p_id       = malloc(sizeof(struct ItemDef));
            * * p_p_id     = id;
            p_p_id         = (struct ItemDef    * *) * p_p_id;
        }
        else
        {
            md.map_obj_def = mod;
            * p_p_md       = malloc(sizeof(struct MonsterDef));
            * * p_p_md     = md;
            p_p_md         = (struct MonsterDef * *) * p_p_md;
        }
    }
    free(defline);
    fclose(file);
};



extern void write_map_objects(void * start, FILE * file,
                              void (* w_typedata) (void *, FILE *) )
{
    struct MapObj * map_obj;
    for (map_obj = start; map_obj != 0; map_obj = map_obj->next)
    {
        write_uint16_bigendian(map_obj->pos.y + 1, file);
        write_uint16_bigendian(map_obj->pos.x + 1, file);
        fputc(map_obj->type, file);
        if (w_typedata)
        {
            w_typedata (map_obj, file);
        }
    }
    write_uint16_bigendian(0, file);
}



extern void read_map_objects(void * start, FILE * file, size_t size,
                             void (* r_typedata) (void *, FILE *) )
{
    struct MapObj * map_obj;
    uint16_t test;
    char first = 1;
    while (1)
    {
        test = read_uint16_bigendian(file);
        if (0 == test)
        {
            break;
        }
        map_obj = get_next_map_obj(start, &first, size, map_obj);
        map_obj->pos.y = test - 1;
        map_obj->pos.x = read_uint16_bigendian(file) - 1;
        map_obj->type = fgetc(file);
        if (r_typedata)
        {
            r_typedata(map_obj, file);
        }
    }
    if (!first)
    {
        map_obj->next = 0;
    }
}



extern void write_map_objects_monsterdata(void * start, FILE * file)
{
  struct Monster * m = (struct Monster *) start;
  fputc(m->hitpoints, file);
}



extern void read_map_objects_monsterdata (void * start, FILE * file)
{
  struct Monster * m = (struct Monster *) start;
  m->hitpoints = fgetc(file);
}



extern void * build_map_objects(struct World * world, void * start, char def_id,
                                unsigned char n, size_t size,
                                void (* b_typedata) (struct MapObjDef *,
                                                     void *))
{
    unsigned char i;
    struct MapObj * mo;
    char first = 1;
    struct MapObjDef * mod = get_map_obj_def(world, def_id);
    for (i = 0; i < n; i++)
    {
        mo = get_next_map_obj(start, &first, size, mo);
        mo->pos = find_passable_pos(world->map);
        b_typedata(mod, mo);
    }
    if (!first)
    {
        mo->next = 0;
    }
    return &mo->next;
}



extern void build_map_objects_itemdata(struct MapObjDef * map_obj_def,
                                       void * start)
{
  struct Item * i = (struct Item *) start;
  i->map_obj.type = map_obj_def->id;
}



extern void build_map_objects_monsterdata(struct MapObjDef * map_obj_def,
                                          void * start)
{
    struct Monster * m = (struct Monster *) start;
    m->map_obj.type = map_obj_def->id;
    struct MonsterDef * md = (struct MonsterDef *) map_obj_def;
    m->hitpoints = md->hitpoints_start;
}



extern struct MapObjDef * get_map_obj_def (struct World * world, char def_id)
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
