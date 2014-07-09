/* src/server/configfile.c */

#include <stddef.h> /* size_t, NULL */
#include <stdio.h> /* snprintf() */
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* atoi(), free() */
#include <string.h> /* strcmp() */
#include "../common/parse_file.h" /* EDIT_STARTED, parsetest_int(),parse_file(),
                                   * parsetest_too_many_values(),parse_id_uniq()
                                   * parse_unknown_arg(), parse_init_entry(),
                                   * parse_and_reduce_to_readyflag(),
                                   * parse_flagval()
                                   */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag(), CLEANUP_THING_TYPES,
                      * CLEANUP_THING_ACTIONS
                      */
#include "hardcoded_strings.h" /* s */
#include "thing_actions.h" /* ThingAction */
#include "things.h" /* Thing, ThingType */
#include "world.h" /* world global */



/* Flags defining state of thing type and action entry reading ((un-)finished /
 * ready for starting the reading of a new definition etc.)
 */
enum flag
{
    HEIGHT_SET     = 0x02,
    WIDTH_SET      = 0x04,
    NAME_SET       = 0x02,
    EFFORT_SET     = 0x04,
    CORPSE_ID_SET  = 0x04,
    SYMBOL_SET     = 0x08,
    LIFEPOINTS_SET = 0x10,
    CONSUMABLE_SET = 0x20,
    START_N_SET    = 0x40,
    READY_ACTION   = NAME_SET | EFFORT_SET,
    READY_THING    = NAME_SET | CORPSE_ID_SET | SYMBOL_SET | LIFEPOINTS_SET
                     | CONSUMABLE_SET | START_N_SET
};



/* What ThingType and ThingAction structs have in common at their top. Use this
 * to allow same functions run over structs of both types.
 */
struct EntryHead
{
    uint8_t id;
    struct EntryHead * next;
};



/* Interpret "token0" and "token1" as data to write into the ThingAction /
 * ThingType DB.
 *
 * Individual ThingType / ThingAction DB entries are put together line by line
 * before being written. Writing only happens after all necessary members of an
 * entry have been assembled, and when additionally a) a new entry is started by
 * a "token0" of "ACTION" or "THINGTYPE"; or b) "token0" is NULL.
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

/* Write DB entry pointed to by "entry" to its appropriate location. */
static void write_if_entry(struct EntryHead ** entry,
                           struct EntryHead *** entry_p_p_p);

/* Ensure that all .corpse_id members in the ThingType DB fit .id members of
 * ThingType DB entries.
 */
static void test_corpse_ids();

/* If "token0" matches "comparand", set world.player_type to int in "token1". */
static uint8_t set_player_type(char * token0, char * comparand, char * token1);

/* If "token0" matches "comparand", set world.map.length to int in "token1". */
static uint8_t set_map_length(char * token0, char * comparand, char * token1);

/* Try to read tokens as members for the definition currently edited, which may
 * be "tt" or "ta". What member of which of the two is set depends on which of
 * the flags has EDIT_STARTED set and on the key name in "token0". Return 1 if
 * interpretation succeeds, else 0.
 *
 * Note that ThingAction entries' .name also determines their .func.
 */
static uint8_t set_members(char * token0, char * token1,
                           uint8_t * thing_flags, uint8_t * action_flags,
                           struct ThingType * tt, struct ThingAction * ta);

/* If "name" fits "ta"->name, set "ta"->func to "func". (Derives ThingAction
 * .func from .name for set_members().
 */
static uint8_t try_func_name(struct ThingAction * ta,
                             char * name, void (* func) (struct Thing *));



static void tokens_into_entries(char * token0, char * token1)
{
    char * str_action = "ACTION";
    char * str_thing = "THINGTYPE";
    char * str_player = "PLAYER_TYPE";
    char * str_map_length = "MAP_LENGTH";
    static struct ThingAction ** ta_p_p = &world.thing_actions;
    static struct ThingType ** tt_p_p = &world.thing_types;
    static uint8_t action_flags = READY_ACTION;
    static uint8_t thing_flags = READY_THING;
    static struct EntryHead * ta = NULL;
    static struct EntryHead * tt = NULL;
    if (!token0 || !strcmp(token0, str_action) || !strcmp(token0, str_thing)
                || !strcmp(token0, str_player))
    {
        parse_and_reduce_to_readyflag(&action_flags, READY_ACTION);
        parse_and_reduce_to_readyflag(&thing_flags, READY_THING);
        write_if_entry(&ta, (struct EntryHead ***) &ta_p_p);
        write_if_entry(&tt, (struct EntryHead ***) &tt_p_p);
    }
    if (token0)
    {
        parsetest_too_many_values();
        if (start_entry(token0, token1, str_action, &action_flags,
                        sizeof(struct ThingAction), (struct EntryHead**) &ta,
                        (struct EntryHead *) world.thing_actions))
        {
            err_line(0 == atoi(token1), "Value must not be 0.");
        }
        else if (!(   start_entry(token0, token1, str_thing, &thing_flags,
                                  sizeof(struct ThingType),
                                  (struct EntryHead**) &tt,
                                  (struct EntryHead *) world.thing_types)
                   || set_player_type(token0, str_player, token1)
                   || set_map_length(token0, str_map_length, token1)
                   || set_members(token0, token1, &thing_flags, &action_flags,
                                  (struct ThingType *) tt,
                                  (struct ThingAction *) ta)))
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
    char * prefix = "In the thing types DB, one thing corpse ID does not "
                    "reference any known thing type in the DB. ID of "
                    "responsible thing type: ";
    size_t size = strlen(prefix) + 3 + 1; /* 3: uint8_t representation strlen */
    char * err_corpse = try_malloc(size, f_name);
    struct ThingType * test_entry_0 = world.thing_types;
    for (; test_entry_0; test_entry_0 = test_entry_0->next)
    {
        uint8_t corpse_id_found = 0;
        struct ThingType * test_entry_1 = world.thing_types;
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



static uint8_t set_player_type(char * token0, char * comparand, char * token1)
{
    if (strcmp(token0, comparand))
    {
        return 0;
    }
    parsetest_int(token1, '8');
    world.player_type = atoi(token1);
    return 1;
}



static uint8_t set_map_length(char * token0, char * comparand, char * token1)
{
    if (strcmp(token0, comparand))
    {
        return 0;
    }
    parsetest_int(token1, 'i');
    int test = atoi(token1) > 256 || atoi(token1) < 1;
    err_line(test, "Value must be >= 1 and <= 256.");
    world.map.length = atoi(token1);
    return 1;
}



static uint8_t set_members(char * token0, char * token1, uint8_t * thing_flags,
                           uint8_t * action_flags,
                           struct ThingType * tt, struct ThingAction * ta)
{
    if (   *action_flags & EDIT_STARTED
        && parse_flagval(token0, token1, "NAME", action_flags,
                         NAME_SET, 's', (char *) &ta->name))
    {
        if (!(   try_func_name(ta, s[S_CMD_MOVE], actor_move)
              || try_func_name(ta, s[S_CMD_PICKUP], actor_pick)
              || try_func_name(ta, s[S_CMD_DROP], actor_drop)
              || try_func_name(ta, s[S_CMD_USE], actor_use)))
        {
            ta->func = actor_wait;
        }
        *action_flags = *action_flags | NAME_SET;
        return 1;
    }
    else if (   parse_flagval(token0, token1, "NAME", thing_flags,
                              NAME_SET, 's', (char *) &tt->name)
             || parse_flagval(token0, token1, "SYMBOL", thing_flags,
                              SYMBOL_SET, 'c', (char *) &tt->char_on_map)
             || parse_flagval(token0, token1, "EFFORT", action_flags,
                              EFFORT_SET, '8', (char *) &ta->effort)
             || parse_flagval(token0, token1, "START_NUMBER", thing_flags,
                              START_N_SET, '8', (char *) &tt->start_n)
             || parse_flagval(token0, token1, "LIFEPOINTS", thing_flags,
                              LIFEPOINTS_SET, '8', (char *) &tt->lifepoints)
             || parse_flagval(token0, token1, "CONSUMABLE", thing_flags,
                              CONSUMABLE_SET, '8', (char *) &tt->consumable)
             || parse_flagval(token0, token1, "CORPSE_ID", thing_flags,
                              CORPSE_ID_SET, '8', (char *) &tt->corpse_id))
    {
        return 1;
    }
    return 0;
}



static uint8_t try_func_name(struct ThingAction * ta, char * name,
                             void (* func) (struct Thing *))
{
    if (0 == strcmp(ta->name, name))
    {
        ta->func = func;
        return 1;
    }
    return 0;
}



extern void read_config_file()
{
    parse_file(s[S_PATH_CONFIG], tokens_into_entries);
    exit_err(!world.map.length, "Map size not defined in config file.");
    uint8_t player_type_is_valid = 0;
    struct ThingType * tt;
    for (tt = world.thing_types; NULL != tt; tt = tt->next)
    {
        if (world.player_type == tt->id)
        {
            player_type_is_valid = 1;
            break;
        }
    }
    exit_err(!player_type_is_valid, "No valid thing type set for player.");
    set_cleanup_flag(CLEANUP_THING_ACTIONS | CLEANUP_THING_TYPES);
    test_corpse_ids();
}
