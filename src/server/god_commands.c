/* src/server/god_commands.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "god_commands.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* atoi(), free() */
#include <string.h> /* strcmp(), memset(), memcpy() */
#include <unistd.h> /* F_OK, access(), unlink() */
#include "../common/parse_file.h" /* err_line(), parse_val(), parsetest_int() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* unset_cleanup_flag() */
#include "field_of_view.h" /* build_fov_map() */
#include "hardcoded_strings.h" /* s */
#include "init.h" /* remake_world() */
#include "map.h" /* remake_map() */
#include "thing_actions.h" /* ThingAction, actor_wait(), actor_move(),
                            * actor_use(), actor_pickup(), actor_drop()
                            */
#include "things.h" /* Thing, ThingType, add_thing(), get_thing(), own_thing(),
                     * free_things(), add_thing_to_memory_map(),get_thing_type()
                     */
#include "world.h" /* world */



/* Parse/apply god command in "tok0"/tok1" to manipulate a ThingType*/
static uint8_t parse_thingtype_manipulation(char * tok0, char * tok1);

/* If "name" fits "ta"->name, set "ta"->func to "func". (Derives ThingAction
 * .func from .name for set_members().
 */
static uint8_t try_func_name(struct ThingAction * ta, char * name,
                             void (* func) (struct Thing *));

/* Parse/apply god command in "tok0"/"tok1" to manipulate a ThingAction. */
static uint8_t parse_thingaction_manipulation(char * tok0, char * tok1);

/* Parse/apply god command in "tok0"/"tok1" oo setting "t"'s thing type. */
static uint8_t parse_thing_type(char * tok0, char * tok1, struct Thing * t);

/* Parse/apply god command in "tok0"/"tok1" on setting up thing "t". */
static uint8_t parse_thing_command(char * tok0, char * tok1, struct Thing * t);

/* Parse/apply god command in "tok0"/"tok1" on positioning a thing "t". */
static uint8_t parse_position(char* tok0, char * tok1, struct Thing * t);

/* Parse/apply god command in "tok0"/"tok1" on "t" owning another thing. */
static uint8_t parse_carry(char * tok0, char * tok1, struct Thing * t);

/* Parse/apply god command in "tok0"/"tok1" to manipulate a Thing. */
static uint8_t parse_thing_manipulation_1arg(char * tok0, char * tok1);

/* Performs parse_world_active()'s world activation legality tests. */
static uint8_t world_may_be_set_active();

/* Unlink worldstate file if it exists. */
static void remove_worldstate_file();

/* Parse/apply god command in "tok0"/"tok1" on toggling world.exists. Unset if
 * argument is 0 and unlink worldstate file, but only set it on positive
 * argument if it is not already set and a thing action of name S_CMD_WAIT, a
 * player thing and a map are defined. On setting it, rebuild all FOVs.
 */
static uint8_t parse_world_active(char * tok0, char * tok1);

/* Parse/apply god command in "tok0"/"tok1" to reset world.map.length. On
 * re-set, set world.exists to 0, remove all things and free world.map.cells
 */
static uint8_t set_map_length(char * tok0, char * tok1);



/* Thing, ThingType or ThingAction selected to be manipulated. */
static struct Thing * t = NULL;
static struct ThingType * tt = NULL;
static struct ThingAction * ta = NULL;



static uint8_t parse_thingtype_manipulation(char * tok0, char * tok1)
{
    if (!tt &&
        (   !strcmp(tok0, s[S_CMD_TT_CONSUM]) || !strcmp(tok0, s[S_CMD_TT_SYMB])
         || !strcmp(tok0, s[S_CMD_TT_STARTN]) || !strcmp(tok0, s[S_CMD_TT_NAME])
         || !strcmp(tok0, s[S_CMD_TT_CORPS]) || !strcmp(tok0, s[S_CMD_TT_HP])))
    {
        return err_line(1, "No thing type defined to manipulate yet.");
    }
    int16_t id;
    if (   parse_val(tok0,tok1,s[S_CMD_TT_CONSUM],'8',(char *) &tt->consumable)
        || parse_val(tok0,tok1,s[S_CMD_TT_HP],'8',(char *) &tt->lifepoints)
        || parse_val(tok0,tok1,s[S_CMD_TT_STARTN],'8',(char *) &tt->start_n)
        || parse_val(tok0,tok1,s[S_CMD_TT_SYMB],'c',(char *) &tt->char_on_map)
        || parse_val(tok0,tok1,s[S_CMD_TT_NAME],'s',(char *) &tt->name));
    else if (parse_val(tok0, tok1, s[S_CMD_TT_CORPS],'8',(char *)&id))
    {
        if (!get_thing_type(id))
        {
            return err_line(1, "Corpse ID belongs to no known thing type.");
        }
        tt->corpse_id = id;
    }
    else if (parse_val(tok0, tok1, s[S_CMD_TT_ID], 'i', (char *) &id))
    {
        tt = get_thing_type(id);
        if (!tt)
        {
            tt = add_thing_type(id);
        }
    }
    else
    {
        return 0;
    }
    return 1;
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



static uint8_t parse_thingaction_manipulation(char * tok0, char * tok1)
{
    if (!ta &&
        (!strcmp(tok0, s[S_CMD_TA_EFFORT]) || !strcmp(tok0, s[S_CMD_TA_NAME])))
    {
        return err_line(1, "No thing action defined to manipulate yet.");
    }
    int16_t id;
    if      (parse_val(tok0, tok1, s[S_CMD_TA_EFFORT],'8',(char *)&ta->effort));
    else if (parse_val(tok0, tok1, s[S_CMD_TA_NAME], 's', (char *)&ta->name))
    {
        if (!(   try_func_name(ta, s[S_CMD_MOVE], actor_move)
              || try_func_name(ta, s[S_CMD_PICKUP], actor_pick)
              || try_func_name(ta, s[S_CMD_WAIT], actor_wait)
              || try_func_name(ta, s[S_CMD_DROP], actor_drop)
              || try_func_name(ta, s[S_CMD_USE], actor_use)))
        {
            return err_line(1, "Invalid action function name.");
        }
        if (world.exists)
        {         /* Legal worlds have at least one thing action for waiting. */
            world.exists = 0 != get_thing_action_id_by_name(s[S_CMD_WAIT]);
            if (!world.exists)
            {
                remove_worldstate_file();
            }
        }
    }
    else if (parse_val(tok0, tok1, s[S_CMD_TA_ID], '8', (char *) &id))
    {
        ta = get_thing_action(id);
        if (!ta)
        {
            ta = add_thing_action(id);
        }
    }
    else
    {
        return 0;
    }
    return 1;
}



static uint8_t parse_thing_type(char * tok0, char * tok1, struct Thing * t)
{
    uint8_t type;
    if (parse_val(tok0, tok1, s[S_CMD_T_TYPE], '8', (char *) &type))
    {
        struct ThingType * tt = get_thing_type(type);
        if (!err_line(!tt, "Thing type does not exist."))
        {
            t->type = type;
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_thing_command(char * tok0, char * tok1, struct Thing * t)
{
    uint8_t command;
    if (parse_val(tok0, tok1, s[S_CMD_T_COMMAND], '8', (char *) &command))
    {
        if (!command)
        {
            t->command = command;
            return 1;
        }
        struct ThingAction * ta = world.thing_actions;
        for (; ta && command != ta->id; ta = ta->next);
        if (!err_line(!ta, "Thing action does not exist."))
        {
            t->command = command;
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_position(char* tok0, char * tok1, struct Thing * t)
{
    char axis = 0;
    if      (!strcmp(tok0, s[S_CMD_T_POSY]))
    {
        axis = 'y';
    }
    else if (!strcmp(tok0, s[S_CMD_T_POSX]))
    {
        axis = 'x';
    }
    if (axis && !parsetest_int(tok1, '8'))
    {
        uint8_t length = atoi(tok1);
        char * err = "Position is outside of map.";
        if (!err_line(length >= world.map.length, err))
        {
            if      ('y' == axis)
            {
                t->pos.y = length;
            }
            else if ('x' == axis)
            {
                t->pos.x = length;
            }
            if (world.exists && t->lifepoints)
            {
                build_fov_map(t);
            }
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_carry(char * tok0, char * tok1, struct Thing * t)
{
    uint8_t id;
    if (parse_val(tok0, tok1, s[S_CMD_T_CARRIES], '8', (char *) &id))
    {
        if (!err_line(id == t->id, "Thing cannot carry itself."))
        {
            struct Thing * o = get_thing(world.things, id, 0);
            if (!err_line(!o, "Thing not available for carrying."))
            {
                own_thing(&(t->owns), &world.things, id);
                o->pos = t->pos;
            }
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_thing_manipulation_1arg(char * tok0, char * tok1)
{
    if (!t &&
        (   !strcmp(tok0, s[S_CMD_T_PROGRESS]) || !strcmp(tok0, s[S_CMD_T_TYPE])
         || !strcmp(tok0, s[S_CMD_T_CARRIES]) || !strcmp(tok0, s[S_CMD_T_POSY])
         || !strcmp(tok0, s[S_CMD_T_POSY]) || !strcmp(tok0, s[S_CMD_T_ARGUMENT])
         || !strcmp(tok0, s[S_CMD_T_HP]) || !strcmp(tok0, s[S_CMD_T_COMMAND])))
    {
        return err_line(1, "No thing defined to manipulate yet.");
    }
    int16_t id;
    if (   parse_thing_type(tok0, tok1, t)
        || parse_thing_command(tok0, tok1, t)
        || parse_val(tok0,tok1, s[S_CMD_T_ARGUMENT], '8', (char *)&t->arg)
        || parse_val(tok0,tok1, s[S_CMD_T_PROGRESS], '8', (char *)&t->progress)
        || parse_val(tok0,tok1, s[S_CMD_T_HP], '8', (char *) &t->lifepoints)
        || parse_position(tok0, tok1, t)
        || parse_carry(tok0, tok1, t));
    else if (parse_val(tok0, tok1, s[S_CMD_T_ID], 'i', (char *) &id))
    {
        t = get_thing(world.things, id, 1);
        char * err = "No thing type found to initialize new thing.";
        if (!t && !err_line(!world.thing_types, err))
        {
            t = add_thing(id, world.thing_types->id, 0, 0);
            if (world.exists && t->lifepoints)
            {
                build_fov_map(t);
            }
        }
    }
    else
    {
        return 0;
    }
    return 1;
}



static uint8_t world_may_be_set_active()
{
    if (!get_thing_action_id_by_name(s[S_CMD_WAIT]))
    {
        err_line(1, "No thing action of name 'wait' found.");
        return 0;
    }
    if (!get_player())
    {
        err_line(1, "No un-owned player thing (of id=0) found.");
        return 0;
    }
    if (!world.map.cells)
    {
        err_line(1, "No map found.");
        return 0;
    }
    return 1;
}



static void remove_worldstate_file()
{
    if (!access(s[S_PATH_WORLDSTATE], F_OK))
    {
        int test = unlink(s[S_PATH_WORLDSTATE]);
        exit_trouble(-1 == test, __func__, "unlink");
    }
}



static uint8_t parse_world_active(char * tok0, char * tok1)
{
    if (!strcmp(tok0, s[S_CMD_WORLD_ACTIVE]) && !parsetest_int(tok1, '8'))
    {
        if (!parsetest_int(tok1, '8'))
        {
            uint8_t argument = atoi(tok1);
            if (!argument)
            {
                remove_worldstate_file();
                world.exists = 0;
            }
            else if (world.exists)
            {
                err_line(1, "World already active.");
            }
            else if (world_may_be_set_active())
            {
                struct Thing * ti;
                for (ti = world.things; ti; ti = ti->next)
                {
                    if (ti->lifepoints)
                    {
                        build_fov_map(ti);
                    }
                }
                world.exists = 1;
            }
            return 1;
        }
    }
    return 0;
}



static uint8_t set_map_length(char * tok0, char * tok1)
{
    if (!strcmp(tok0, s[S_CMD_MAPLENGTH]) && !parsetest_int(tok1, 'u'))
    {
        uint16_t argument = atoi(tok1);
        if (argument < 1 || argument > 256)
        {
            return err_line(1, "Value must be >= 1 and <= 256.");
        }
        world.exists = 0;
        remove_worldstate_file();
        free_things(world.things);
        free(world.map.cells);
        world.map.cells = NULL;    /* Since remake_map() runs free() on this. */
        world.map.length = argument;
        return 1;
    }
    return 0;
}



extern uint8_t parse_god_command_1arg(char * tok0, char * tok1)
{
    if (   parse_thingtype_manipulation(tok0, tok1)
        || parse_thingaction_manipulation(tok0, tok1)
        || parse_thing_manipulation_1arg(tok0, tok1)
        || set_map_length(tok0,tok1)
        || parse_val(tok0,tok1,s[S_CMD_SEED_RAND],'U', (char *)&world.seed)
        || parse_val(tok0,tok1,s[S_CMD_TURN],'u',(char *)&world.turn)
        || parse_val(tok0,tok1,s[S_CMD_PLAYTYPE],'8',(char *)&world.player_type)
        || parse_world_active(tok0, tok1));
    else if (parse_val(tok0,tok1,s[S_CMD_SEED_MAP],'U',(char *)&world.seed_map))

    {
        remake_map();
    }
    else if (parse_val(tok0, tok1, s[S_CMD_MAKE_WORLD],'U',(char *)&world.seed))
    {
        uint8_t test = remake_world();
        err_line(1 == test, "No player type with start number of >0 defined.");
        err_line(2 == test, "No thing action with name 'wait' defined.");
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t parse_god_command_2arg(char * tok0, char * tok1, char * tok2)
{
    if (!t && !strcmp(tok0, s[S_CMD_T_MEMMAP]))
    {
        return err_line(1, "No thing defined to manipulate yet.");
    }
    if (!strcmp(tok0, s[S_CMD_T_MEMMAP]))
    {
        uint8_t y = atoi(tok1);
        if (parsetest_int(tok1, '8') || y >= world.map.length)
        {
            return err_line(1, "Illegal value for map line number.");
        }
        if (strlen(tok2) != world.map.length)
        {
            return err_line(1, "Map line length is unequal map width.");
        }
        if (!t->mem_map)
        {
            uint32_t map_size = world.map.length * world.map.length;
            t->mem_map = try_malloc(map_size, __func__);
            memset(t->mem_map, ' ', map_size);
        }
        memcpy(t->mem_map + y * world.map.length, tok2, world.map.length);
    }
    else
    {
        return 0;
    }
    return 1;
}



extern uint8_t parse_god_command_3arg(char * tok0, char * tok1, char * tok2,
                                      char * tok3)
{
    if (!t && !strcmp(tok0, s[S_CMD_T_MEMTHING]))
    {
        return err_line(1, "No thing defined to manipulate yet.");
    }
    if (!strcmp(tok0, s[S_CMD_T_MEMTHING]))
    {
        uint8_t id = atoi(tok1);
        uint8_t y  = atoi(tok2);
        uint8_t x  = atoi(tok3);
        if (   parsetest_int(tok1, '8') || !get_thing_type(id)
            || parsetest_int(tok2, '8') || y >= world.map.length
            || parsetest_int(tok3, '8') || x >= world.map.length)
        {
            return err_line(1, "Illegal value for thing type or position.");
        }
        add_thing_to_memory_map(t, id, y, x);
    }
    else
    {
        return 0;
    }
    return 1;
}
