/* src/server/configfile.c */

#include <stddef.h> /* size_t, NULL */
#include <stdio.h> /* snprintf() */
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* atoi(), free() */
#include <string.h> /* strcmp() */
#include "../common/parse_file.h" /* EDIT_STARTED, parsetest_int(),parse_file(),
                                   * parsetest_too_many_values(),parse_id_uniq()
                                   * parse_unknown_arg(), parse_init_entry(),
                                   * parse_and_reduce_to_readyflag(),parse_val()
                                   */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag(), CLEANUP_MAP_OBJ_DEFS,
                      * CLEANUP_MAP_OBJ_ACTS
                      */
#include "map_object_actions.h" /* struct MapObjAct */
#include "map_objects.h" /* struct MapObj, struct MapObjDef */
#include "world.h" /* world global */



/* Flags defining state of object and action entry reading ((un-)finished /
 * ready for starting the reading of a new definition etc.)
 */
enum flag
{
    HEIGHT_SET     = 0x02,
    WIDTH_SET      = 0x04,
    ORTH_SET       = 0x08,
    DIAG_SET       = 0x10,
    NAME_SET       = 0x02,
    EFFORT_SET     = 0x04,
    CORPSE_ID_SET  = 0x04,
    SYMBOL_SET     = 0x08,
    LIFEPOINTS_SET = 0x10,
    CONSUMABLE_SET = 0x20,
    READY_ACT = NAME_SET | EFFORT_SET,
    READY_OBJ = NAME_SET | CORPSE_ID_SET | SYMBOL_SET | LIFEPOINTS_SET
                | CONSUMABLE_SET,
    READY_MAP = HEIGHT_SET | WIDTH_SET | ORTH_SET | DIAG_SET
};



/* What MapObjDef and MapObjAct structs have in common at their top. Use this to
 * allow same functions run over structs of both types.
 */
struct EntryHead
{
    uint8_t id;
    struct EntryHead * next;
};



/* Interpret "token0" and "token1" as data to write into the MapObjAct /
 * MapObjDef DB.
 *
 * Individual MapObjDef / MapObjAct DB entries are put together line by line
 * before being written. Writing only happens after all necessary members of an
 * entry have been assembled, and when additionally a) a new entry is started by
 * a "token0" of "ACTION" or "OBJECT"; or b) "token0" is NULL.
 *
 * Also check against the line parse_file() read tokens from having more tokens.
 */
static void tokens_into_entries(char * token0, char * token1);

/* Start reading a new DB entry of "size" from tokens if "token0" matches
 * "comparand". Set EDIT_STARTED in "flags" to mark beginning of new entry
 * reading. Check that "token1" id of new entry has not already been used in DB
 * starting at "entry_cmp".
 */
static uint8_t start_entry(char * token0, char * token1, char * comparand,
                         uint8_t * flags, size_t size,
                         struct EntryHead ** entry,
                         struct EntryHead * entry_cmp);

/* Start reading map definition if "token0" matches "comparand". Set "map_flags"
 * to EDIT_STARTED to mark beginning of map definition reading.
 */
static uint8_t start_map(char * token0, char * comparand, uint8_t * map_flags);

/* Write DB entry pointed to by "entry" to its appropriate location. */
static void write_if_entry(struct EntryHead ** entry,
                           struct EntryHead *** entry_p_p_p);

/* Ensure that all .corpse_id members in the MapObjDef DB fit .id members of
 * MapObjDef DB entries.
 */
static void test_corpse_ids();

/* Try to read tokens as members for the definition currently edited, which may
 * be "mod" or "moa" or that of world.map. What member of which of the three is
 * set depends on which of the flags has EDIT_STARTED set and on the key name in
 * "token0". Return 1 if interpretation succeeds, else 0.
 *
 * Note that MapObjAct entries' .name also determines their .func.
 */
static uint8_t set_members(char * token0, char * token1, uint8_t * object_flags,
                           uint8_t * action_flags, uint8_t * map_flags,
                           struct MapObjDef * mod, struct MapObjAct * moa);

/* set_members() helper specifically for editing world.map members. */
static uint8_t set_map_members(char * token0,char * token1,uint8_t * map_flags);

/* If "name" fits "moa"->name, set "moa"->func to "func". (Derives MapObjAct
 * .func from .name for set_members().
 */
static uint8_t try_func_name(struct MapObjAct * moa,
                             char * name, void (* func) (struct MapObj *));



static void tokens_into_entries(char * token0, char * token1)
{
    char * str_act = "ACTION";
    char * str_obj = "OBJECT";
    char * str_map = "MAP_TYPE";
    static struct MapObjAct ** moa_p_p = &world.map_obj_acts;
    static struct MapObjDef ** mod_p_p = &world.map_obj_defs;
    static uint8_t action_flags = READY_ACT;
    static uint8_t object_flags = READY_OBJ;
    static uint8_t map_flags = READY_MAP;
    static struct EntryHead * moa = NULL;
    static struct EntryHead * mod = NULL;
    if (!token0 || !strcmp(token0, str_act) || !strcmp(token0, str_obj)
                || !strcmp(token0, str_map))
    {
        parse_and_reduce_to_readyflag(&action_flags, READY_ACT);
        parse_and_reduce_to_readyflag(&object_flags, READY_OBJ);
        parse_and_reduce_to_readyflag(&map_flags, READY_MAP);
        write_if_entry(&moa, (struct EntryHead ***) &moa_p_p);
        write_if_entry(&mod, (struct EntryHead ***) &mod_p_p);
    }
    if (token0)
    {
        parsetest_too_many_values();
        if (!(   start_entry(token0, token1, str_act, &action_flags,
                             sizeof(struct MapObjAct),(struct EntryHead**) &moa,
                             (struct EntryHead *) world.map_obj_acts)
              || start_entry(token0, token1, str_obj, &object_flags,
                             sizeof(struct MapObjDef),(struct EntryHead**) &mod,
                             (struct EntryHead *) world.map_obj_defs)
              || start_map(token0, str_map, &map_flags)
              || set_members(token0, token1, &object_flags, &action_flags,
                              &map_flags, (struct MapObjDef *)mod,
                              (struct MapObjAct *) moa)))
        {
            parse_unknown_arg();
        }
    }
}



static uint8_t start_entry(char * token0, char * token1, char * comparand,
                           uint8_t * flags, size_t size,
                           struct EntryHead ** entry,
                           struct EntryHead * entry_cmp)
{
    if (strcmp(token0, comparand))
    {
        return 0;
    }
    *entry = (struct EntryHead *) parse_init_entry(flags, size);
    parsetest_int(token1, '8');
    (*entry)-> id = atoi(token1);
    for (; NULL != entry_cmp; entry_cmp = entry_cmp->next)
    {
        parse_id_uniq((*entry)->id == entry_cmp->id);
    }
    return 1;
}



static uint8_t start_map(char * token0, char * comparand, uint8_t * map_flags)
{
    if (strcmp(token0, comparand))
    {
        return 0;
    }
    *map_flags = EDIT_STARTED;
    return 1;
}



static void write_if_entry(struct EntryHead ** entry,
                           struct EntryHead *** entry_p_p_p)
{
    if (*entry)
    {
        (*entry)->next = NULL;
        **entry_p_p_p = *entry;
        *entry_p_p_p = &((*entry)->next);
        *entry = NULL;   /* So later runs of this don't re-append same entry. */
    }
}



static void test_corpse_ids()
{
    char * f_name = "test_corpse_ids()";
    char * prefix = "In the object definitions DB, one object corpse ID does "
                    "not reference any known object in the DB. ID of "
                    "responsible object: ";
    size_t size = strlen(prefix) + 3 + 1; /* 3: uint8_t representation strlen */
    char * err_corpse = try_malloc(size, f_name);
    struct MapObjDef * test_entry_0 = world.map_obj_defs;
    for (; test_entry_0; test_entry_0 = test_entry_0->next)
    {
        uint8_t corpse_id_found = 0;
        struct MapObjDef * test_entry_1 = world.map_obj_defs;
        for (; test_entry_1; test_entry_1 = test_entry_1->next)
        {
            if (test_entry_0->corpse_id == test_entry_1->id)
            {
                corpse_id_found = 1;
            }
        }
        int test = snprintf(err_corpse, size, "%s%d", prefix, test_entry_0->id);
        exit_trouble(test < 0, f_name, "snprintf()");
        exit_err(!corpse_id_found, err_corpse);
    }
    free(err_corpse);
}



static uint8_t set_map_members(char * token0, char * token1, uint8_t * map_flags)
{
    if      (   parse_val(token0, token1, "HEIGHT", map_flags,
                          HEIGHT_SET, 'i', (char *) &world.map.size.y)
             || parse_val(token0, token1, "WIDTH", map_flags,
                          WIDTH_SET, 'i', (char *) &world.map.size.x))
    {
        int test = atoi(token1) > 256 || atoi(token1) < 1;
        err_line(test, "Value must be >= 1 and <= 256.");
        return 1;
    }
    else if (    parse_val(token0, token1, "DIST_ORTHOGONAL", map_flags,
                          ORTH_SET, '8', (char *) &world.map.dist_orthogonal)
             ||  parse_val(token0, token1, "DIST_DIAGONAL", map_flags,
                          DIAG_SET, '8', (char *) &world.map.dist_diagonal))
    {
        err_line(0 == atoi(token1), "Value must not be zero.");
        return 1;
    }
    return 0;
}



static uint8_t set_members(char * token0, char * token1, uint8_t * object_flags,
                           uint8_t * action_flags, uint8_t * map_flags,
                           struct MapObjDef * mod, struct MapObjAct * moa)
{
    if (   *action_flags & EDIT_STARTED
        && parse_val(token0, token1, "NAME", action_flags,
                     NAME_SET, 's', (char *) &moa->name))
    {
        if (!(   try_func_name(moa, "move", actor_move)
              || try_func_name(moa, "pick_up", actor_pick)
              || try_func_name(moa, "drop", actor_drop)
              || try_func_name(moa, "use", actor_use)))
        {
            moa->func = actor_wait;
        }
        *action_flags = *action_flags | NAME_SET;
        return 1;
    }
    else if (   set_map_members(token0, token1, map_flags)
             || parse_val(token0, token1, "NAME", object_flags,
                          NAME_SET, 's', (char *) &mod->name)
             || parse_val(token0, token1, "SYMBOL", object_flags,
                          SYMBOL_SET, 'c', (char *) &mod->char_on_map)
             || parse_val(token0, token1, "EFFORT", action_flags,
                          EFFORT_SET, '8', (char *) &moa->effort)
             || parse_val(token0, token1, "LIFEPOINTS", object_flags,
                          LIFEPOINTS_SET, '8', (char *) &mod->lifepoints)
             || parse_val(token0, token1, "CONSUMABLE", object_flags,
                          CONSUMABLE_SET, '8', (char *) &mod->consumable)
             || parse_val(token0, token1, "CORPSE_ID", object_flags,
                          CORPSE_ID_SET, '8', (char *) &mod->corpse_id))
    {
        return 1;
    }
    return 0;
}



static uint8_t try_func_name(struct MapObjAct * moa, char * name,
                             void (* func) (struct MapObj *))
{
    if (0 == strcmp(moa->name, name))
    {
        moa->func = func;
        return 1;
    }
    return 0;
}



extern void read_config_file()
{
    parse_file(world.path_config, tokens_into_entries);
    exit_err(!world.map.size.y, "Map not defined in config file.");
    set_cleanup_flag(CLEANUP_MAP_OBJECT_ACTS | CLEANUP_MAP_OBJECT_DEFS);
    test_corpse_ids();
}
