/* src/server/configfile.c */

#include <stddef.h> /* size_t, NULL */
#include <stdio.h> /* sprintf() */
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* atoi(), free() */
#include <string.h> /* strcmp() */
#include "../common/err_try_fgets.h" /* err_line() */
#include "../common/parse_file.h" /* Context, EDIT_STARTED, set_val(),
                                   * set_uint8(), parse_file()
                                   */
#include "../common/rexit.h" /* exit_err() */
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
    NAME_SET       = 0x02,
    EFFORT_SET     = 0x04,
    CORPSE_ID_SET  = 0x04,
    SYMBOL_SET     = 0x08,
    LIFEPOINTS_SET = 0x10,
    CONSUMABLE_SET = 0x20,
    READY_ACT = NAME_SET | EFFORT_SET,
    READY_OBJ = NAME_SET | CORPSE_ID_SET | SYMBOL_SET | LIFEPOINTS_SET
                | CONSUMABLE_SET
};



/* What MapObjDef and MapObjAct structs have in common at their top. Used to
 * have common functions run over structs of both types.
 */
struct EntryHead
{
    uint8_t id;
    struct EntryHead * next;
};



/* Get tokens from "context" and, by their order (in the individual context and
 * in subsequent calls of this function), interpret them as data to write into
 * the MapObjAct / MapObjDef DB.
 *
 * Individual MapObjDef / MapObjAct DB entries are put together line by line
 * before being written. Writing only happens after all necessary members of an
 * entry have been assembled, and when additionally a) a new entry is started by
 * a context->token0 of "ACTION" or "OBJECT"; or b) a NULL context->token0 is
 * passed. This is interpreted as the end of the MapObjDef / MapObjAct DB read,
 * so the appropriate cleanup flags are set and test_corpse_ids() is called.
 */
static void tokens_into_entries(struct Context * context);

/* Start reading a new DB entry of "size" from tokens in "context" if ->token0
 * matches "comparand". Set EDIT_STARTED in "flags" to mark beginning of new
 * entry reading. Check that id of new entry in ->token1 has not already been
 * used in DB starting at "entry_cmp".
 */
static uint8_t new_entry(struct Context * context, char * comparand,
                         uint8_t * flags, size_t size,
                         struct EntryHead ** entry,
                         struct EntryHead * entry_cmp);

/* Write DB entry pointed to by "entry" to its appropriate location. */
static void write_if_entry(struct EntryHead ** entry,
                           struct EntryHead *** entry_p_p_p);

/* Ensure that all .corpse_id members in the MapObjDef DB fit .id members of
 * MapObjDef DB entries.
 */
static void test_corpse_ids();

/* Try to read tokens in "context" as members for the entry currently edited,
 * which must be either "mod" or "moa". What member of which of the two is set
 * depends on which of "object_flags" and "action_flags" has EDIT_STARTED set
 * and on the key name of ->token0. Return 1 if interpretation succeeds, else 0.
 *
 * Note that MapObjAct entries' .name also determines their .func.
 */
static uint8_t set_members(struct Context * context, uint8_t * object_flags,
                           uint8_t * action_flags, struct MapObjDef * mod,
                           struct MapObjAct * moa);

/* If "name" fits "moa"->name, set "moa"->func to "func". (Derives MapObjAct
 * .func from .name for set_members().
 */
static uint8_t try_func_name(struct MapObjAct * moa,
                             char * name, void (* func) (struct MapObj *));



static void tokens_into_entries(struct Context * context)
{
    char * str_act = "ACTION";
    char * str_obj = "OBJECT";
    static struct MapObjAct ** moa_p_p = &world.map_obj_acts;
    static struct MapObjDef ** mod_p_p = &world.map_obj_defs;
    static uint8_t action_flags = READY_ACT;
    static uint8_t object_flags = READY_OBJ;
    static struct EntryHead * moa = NULL;
    static struct EntryHead * mod = NULL;
    if (   !context->token0
        || !strcmp(context->token0,str_act) || !strcmp(context->token0,str_obj))
    {
        char * err_fin = "Last definition block not finished yet.";
        err_line((action_flags & READY_ACT) ^ READY_ACT,
                 context->line, context->err_pre, err_fin);
        err_line((object_flags & READY_OBJ) ^ READY_OBJ,
                 context->line, context->err_pre, err_fin);
        write_if_entry(&moa, (struct EntryHead ***) &moa_p_p);
        write_if_entry(&mod, (struct EntryHead ***) &mod_p_p);
        object_flags = action_flags = READY_OBJ;
    }
    if (!context->token0)
    {
        set_cleanup_flag(CLEANUP_MAP_OBJECT_ACTS | CLEANUP_MAP_OBJECT_DEFS);
        test_corpse_ids();
        return;
    }
    if (!(   new_entry(context, str_act, &action_flags,
                       sizeof(struct MapObjAct), (struct EntryHead**) &moa,
                       (struct EntryHead *) world.map_obj_acts)
          || new_entry(context, str_obj, &object_flags,
                       sizeof(struct MapObjDef), (struct EntryHead**) &mod,
                       (struct EntryHead *) world.map_obj_defs)
          || set_members(context, &object_flags, &action_flags,
                         (struct MapObjDef *) mod, (struct MapObjAct *) moa)))
    {
        err_line(1, context->line, context->err_pre, "Unknown argument");
    }
}



static uint8_t new_entry(struct Context * context, char * comparand,
                         uint8_t * flags, size_t size,
                         struct EntryHead ** entry,struct EntryHead * entry_cmp)
{
    char * f_name = "new_entry()";
    char * err_uni = "Declaration of ID already used.";
    if (!strcmp(context->token0, comparand))
    {
        * flags = EDIT_STARTED;
        * entry = try_malloc(size, f_name);
        set_uint8(context, &((*entry)->id));
        for (; NULL != entry_cmp; entry_cmp = entry_cmp->next)
        {
            err_line((*entry)->id == entry_cmp->id,
                     context->line, context->err_pre, err_uni);
        }
        return 1;
    }
    return 0;
}



static void write_if_entry(struct EntryHead ** entry,
                           struct EntryHead *** entry_p_p_p)
{
    if (*entry)
    {
        (* entry)->next = NULL;
        ** entry_p_p_p = *entry;
        * entry_p_p_p = &((*entry)->next);
        * entry = NULL;  /* So later runs of this don't re-append same entry. */
    }
}



static void test_corpse_ids()
{
    char * f_name = "test_corpse_ids()";
    char * err_corpse_prefix = "In the object definition DB, one object corpse "
                               "ID does not reference any known object in the "
                               "DB. ID of responsible object: ";
    char * err_corpse = try_malloc(strlen(err_corpse_prefix) + 3 + 1, f_name);
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
        sprintf(err_corpse, "%s%d", err_corpse_prefix, test_entry_0->id);
        exit_err(!corpse_id_found, err_corpse);
    }
    free(err_corpse);
}



static uint8_t set_members(struct Context * context, uint8_t * object_flags,
                           uint8_t * action_flags, struct MapObjDef * mod,
                           struct MapObjAct * moa)
{
    if (   * action_flags & EDIT_STARTED
        && set_val(context, "NAME", action_flags,
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
    else if (   set_val(context, "NAME", object_flags,
                        NAME_SET, 's', (char *) &mod->name)
             || set_val(context, "SYMBOL", object_flags,
                        SYMBOL_SET, 'c', (char *) &mod->char_on_map)
             || set_val(context, "EFFORT", action_flags,
                        EFFORT_SET, '8', (char *) &moa->effort)
             || set_val(context, "LIFEPOINTS", object_flags,
                        LIFEPOINTS_SET, '8', (char *) &mod->lifepoints)
             || set_val(context, "CONSUMABLE", object_flags,
                        CONSUMABLE_SET, '8', (char *) &mod->consumable)
             || set_val(context, "CORPSE_ID", object_flags,
                        CORPSE_ID_SET, '8', (char *) &mod->corpse_id))
    {
        return 1;
    }
    return 0;
}



static uint8_t try_func_name(struct MapObjAct * moa,
                             char * name, void (* func) (struct MapObj *))
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
}
