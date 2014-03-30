/* src/server/configfile.c */

#define _POSIX_C_SOURCE 200809L /* strdup() */
#include <stddef.h> /* size_t, NULL */
#include <stdio.h> /* FILE, sprintf() */
#include <stdint.h> /* uint8_t, uint32_t */
#include <stdlib.h> /* atoi(), free() */
#include <string.h> /* strchr(), strcmp(), strdup(), strlen() */
#include <unistd.h> /* access(), F_OK */
#include "../common/err_try_fgets.h" /* err_line(), err_try_fgets(),
                                      * reset_err_try_fgets_counter()
                                      */
#include "../common/readwrite.h" /* try_fopen(),try_fclose(),textfile_width() */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag(), CLEANUP_MAP_OBJ_DEFS,
                      * CLEANUP_MAP_OBJ_ACTS
                      */
#include "map_object_actions.h" /* struct MapObjAct */
#include "map_objects.h" /* struct MapObj, struct MapObjDef */
#include "world.h" /* world global */



/* Flags defining state of object and action entry reading ((un-)finished, ready
 * for starting the reading of a new definition etc.
 */
enum flag
{
    EDIT_STARTED   = 0x01,
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



/* Many functions working on config file lines / tokens work with these elements
 * that only change on line change. Makes sense to pass them over together.
 */
struct Context {
    char * line;
    char * token0;
    char * token1;
    char * err_pre;
};



/* Return next token from "line" or NULL if none is found. Tokens either a)
 * start at the first non-whitespace character and end before the next
 * whitespace character after that; or b) if the first non-whitespace character
 * is a single quote followed by at least one other single quote some time later
 * on the line, the token starts after that first single quote and ends before
 * the second, with the next token_from_line() call starting its token search
 * after that second quote. The only way to return an empty string (instead of
 * NULL) as a token is to delimit the token by two succeeding single quotes.
 * */
static char * token_from_line(char * line);

/* Determines the end of the token_from_line() token. */
static void set_token_end(char ** start, char ** limit_char);

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
                         enum flag * flags, size_t size,
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
static uint8_t set_members(struct Context * context, enum flag * object_flags,
                           enum flag * action_flags, struct MapObjDef * mod,
                           struct MapObjAct * moa);

/* If "context"->token0 fits "comparand", set "element" to value read from
 * ->token1 as either string (type: "s"), char ("c") or uint8 ("8"), set
 * that element's flag to "flags" and return 1; else return 0.
 */
static uint8_t set_val(struct Context * context, char * comparand,
                       enum flag * flags, enum flag set_flag, char type,
                       char * element);

/* Writes "context"->token1 to "target" only if it describes a proper uint8. */
static void set_uint8(struct Context * context, uint8_t * target);

/* If "name" fits "moa"->name, set "moa"->func to "func". (Derives MapObjAct
 * .func from .name for set_members().
 */
static uint8_t try_func_name(struct MapObjAct * moa,
                             char * name, void (* func) (struct MapObj *));



static char * token_from_line(char * line)
{
    static char * final_char = NULL;
    static char * limit_char = NULL;
    char * start = limit_char + 1;
    if (line)
    {
        start      = line;
        limit_char = start;
        final_char = &(line[strlen(line)]);
        if ('\n' == *(final_char - 1))
        {
            *(--final_char) = '\0';
        }
    }
    uint8_t empty = 1;
    uint32_t i;
    for (i = 0; '\0' != start[i]; i++)
    {
        if (' ' != start[i] && '\t' != start[i])
        {
            start = &start[i];
            empty = 0;
            break;
        }
    }
    if (empty)
    {
        return start = NULL;
    }
    set_token_end(&start, &limit_char);
    return start;
}



static void set_token_end(char ** start, char ** limit_char)
{
    char * end_quote = ('\'' == (*start)[0]) ? strchr(*start + 1, '\'') : NULL;
    *start = (end_quote) ? *start + 1 : *start;
    if (end_quote)
    {
        *end_quote = '\0';
        *limit_char = end_quote;
        return;
    }
    char * space = strchr(*start, ' ');
    char * tab   = strchr(*start, '\t');
    space = (!space || (tab && tab < space)) ? tab : space;
    if (space)
    {
        * space = '\0';
    }
    *limit_char = strchr(*start, '\0');
}



static void tokens_into_entries(struct Context * context)
{
    char * str_act = "ACTION";
    char * str_obj = "OBJECT";
    static struct MapObjAct ** moa_p_p = &world.map_obj_acts;
    static struct MapObjDef ** mod_p_p = &world.map_obj_defs;
    static enum flag action_flags = READY_ACT;
    static enum flag object_flags = READY_OBJ;
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
        object_flags = action_flags = READY_OBJ;
        write_if_entry(&moa, (struct EntryHead ***) &moa_p_p);
        write_if_entry(&mod, (struct EntryHead ***) &mod_p_p);
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
                         enum flag * flags, size_t size,
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



static uint8_t set_members(struct Context * context, enum flag * object_flags,
                           enum flag * action_flags, struct MapObjDef * mod,
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



static uint8_t set_val(struct Context * context, char * comparand,
                       enum flag * flags, enum flag set_flag, char type,
                       char * element)
{
    char * err_out = "Outside appropriate definition's context.";
    char * err_singlechar = "Value must be single ASCII character.";
    if (!strcmp(context->token0, comparand))
    {
        err_line(!(* flags & EDIT_STARTED),
                 context->line, context->err_pre, err_out);
        * flags = * flags | set_flag;
        if      ('s' == type)
        {
            * (char **) element = strdup(context->token1);
        }
        else if ('c' == type)
        {
            err_line(1 != strlen(context->token1),
                     context->line, context->err_pre, err_singlechar);
            * element = (context->token1)[0];
        }
        else if ('8' == type)
        {
            set_uint8(context, (uint8_t *) element);
        }
        return 1;
    }
    return 0;
}



static void set_uint8(struct Context * context, uint8_t * target)
{
    char * err_uint8 = "Value not unsigned decimal number between 0 and 255.";
    uint8_t i;
    uint8_t is_uint8 = 1;
    for (i = 0; '\0' != context->token1[i]; i++)
    {
        if (i > 2 || '0' > context->token1[i] || '9' < context->token1[i])
        {
            is_uint8 = 0;
        }
    }
    if (is_uint8 && atoi(context->token1) > UINT8_MAX)
    {
        is_uint8 = 0;
    }
    err_line(!(is_uint8), context->line, context->err_pre, err_uint8);
    *target = atoi(context->token1);
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



extern void read_config_file() {
    char * f_name = "read_new_config_file()";
    char * path = world.path_config;
    struct Context context;
    char * err_pre_prefix = "Failed reading config file: \"";
    char * err_pre_affix = "\". ";
    context.err_pre = try_malloc(strlen(err_pre_prefix) + strlen(path)
                                 + strlen(err_pre_affix) + 1, f_name);
    sprintf(context.err_pre, "%s%s%s", err_pre_prefix, path, err_pre_affix);
    exit_err(access(path, F_OK), context.err_pre);
    FILE * file = try_fopen(path, "r", f_name);
    uint32_t linemax = textfile_width(file);
    context.line = try_malloc(linemax + 1, f_name);
    reset_err_try_fgets_counter();
    err_line(0 == linemax, context.line, context.err_pre, "File is empty.");
    context.token0 = NULL; /* For tokens_into_entries() if while() stagnates. */
    char * err_val = "No value given.";
    char * err_many = "Too many values.";
    while (err_try_fgets(context.line, linemax + 1, file, context.err_pre, ""))
    {
        char * line_copy = strdup(context.line);
        context.token0 = token_from_line(line_copy);
        if (context.token0)
        {
            err_line(0 == (context.token1 = token_from_line(NULL)),
                     context.line, context.err_pre, err_val);
            err_line(NULL != token_from_line(NULL),
                     context.line, context.err_pre, err_many);
            tokens_into_entries(&context);
            context.token0 = NULL;
        }
        free(line_copy);
    }
    tokens_into_entries(&context);
    try_fclose(file, f_name);
    free(context.line);
    free(context.err_pre);
}
