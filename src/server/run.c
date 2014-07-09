/* src/server/run.c */

#define _POSIX_C_SOURCE 200809L
#include "run.h"
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE, printf(), sprintf(), fflush() */
#include <stdlib.h> /* free(), atoi() */
#include <string.h> /* strlen(), strcmp() strncmp(), strdup() */
#include <unistd.h> /* access() */
#include "../common/parse_file.h" /* set_err_line_options(), token_from_line(),
                                   * err_line()
                                   */
#include "../common/readwrite.h" /* try_fopen(), try_fcose(), try_fwrite(),
                                  * try_fgets(), try_fclose_unlink_rename(),
                                  * textfile_width(), try_fputc()
                                  */
#include "../common/rexit.h" /* exit_trouble(), exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "ai.h" /* ai() */
#include "cleanup.h" /* set_cleanup_flag(), unset_cleanup_flag() */
#include "field_of_view.h" /* build_fov_map() */
#include "hardcoded_strings.h" /* s */
#include "init.h" /* remake_world() */
#include "io.h" /* io_round(), save_world() */
#include "map.h" /* remake_map() */
#include "thing_actions.h" /* ThingAction */
#include "things.h" /* Thing, get_thing(), own_thing(), add_thing(),
                     * get_thing_action_id_by_name(), get_player()
                     */
#include "world.h" /* global world */



/* Parse/apply god command in "tok0"/"tok1" on "t" owning another thing. */
static uint8_t parse_carry(char * tok0, char * tok1, struct Thing * t);

/* Parse/apply god commansd in "tok0"/"tok1" on positioning a thing "t". */
static uint8_t parse_position(char* tok0, char * tok1, struct Thing * t);

/* Parse/apply god command in "tok0"/"tok1" oo setting "t"'s thing type. */
static uint8_t parse_thing_type(char * tok0, char * tok1, struct Thing * t);

/* Parse/apply god command in "tok0"/"tok1" on setting up thing "t". */
static uint8_t parse_thing_command(char * tok0, char * tok1, struct Thing * t);

/* Parse/apply god command on enabling/disabling generation of fields of view on
 * god commands that may affect them, via static global "do_fov". On enabling,
 * (re-)generate all animate things' fields of view.
 */
static uint8_t parse_do_fov(char * tok0, char * tok1);

/* Parse/apply god command in "tok0"/"tok1" manipulating a thing's state. */
static uint8_t parse_thing_manipulation(char * tok0, char * tok1);

/* Parse player command in "tok0"/"tok1" to action in player thing. */
static uint8_t parse_player_command(char * tok0, char * tok1);

/* Compares first line of server out file to world.server_test, aborts if they
 * don't match, but not before unsetting the flags deleting files in the server
 * directory, for in that case those must be assumed to belong to another server
 * process.
 */
static void server_test();

/* Run the game world and its inhabitants (and their actions) until the player
 * avatar is free to receive new commands (or is dead).
 */
static void turn_over();



/* Do god commands to create / position things generate their fields of view? */
static uint8_t do_fov = 0;



static uint8_t parse_carry(char * tok0, char * tok1, struct Thing * t)
{
    uint8_t id;
    if (parse_val(tok0, tok1, s[S_CMD_CARRIES], '8', (char *) &id))
    {
        if (!err_line(id == t->id, "Thing cannot carry itself."))
        {
            struct Thing * o = get_thing(world.things, id, 0);
            if (!err_line(!o, "Thing cannot carry thing that does not exist."))
            {
                own_thing(&(t->owns), &world.things, id);
                o->pos = t->pos;
            }
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_position(char* tok0, char * tok1, struct Thing * t)
{
    char axis = 0;
    if      (!strcmp(tok0, s[S_CMD_POS_Y]))
    {
        axis = 'y';
    }
    else if (!strcmp(tok0, s[S_CMD_POS_X]))
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
            free(t->fov_map);
            t->fov_map= do_fov && t->lifepoints ? build_fov_map(t) : t->fov_map;
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_thing_type(char * tok0, char * tok1, struct Thing * t)
{
    uint8_t type;
    if (parse_val(tok0, tok1, s[S_CMD_TYPE], '8', (char *) &type))
    {
        struct ThingType * tt = world.thing_types;
        for (; NULL != tt && type != tt->id; tt = tt->next);
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
    if (parse_val(tok0, tok1, s[S_CMD_COMMAND], '8', (char *) &command))
    {
        if (!command)
        {
            t->command = command;
            return 1;
        }
        struct ThingAction * ta = world.thing_actions;
        for (; NULL != ta && command != ta->id; ta = ta->next);
        if (!err_line(!ta, "Thing action does not exist."))
        {
            t->command = command;
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_do_fov(char * tok0, char * tok1)
{
    if (parse_val(tok0, tok1, s[S_CMD_DO_FOV], '8', (char *) &do_fov))
    {
        if (do_fov)
        {
            struct Thing * ti;
            for (ti = world.things; ti; ti = ti->next)
            {
                ti->fov_map = ti->lifepoints ? build_fov_map(ti) : ti->fov_map;
            }
        }
        return 1;
    }
    return 0;
}



static uint8_t parse_thing_manipulation(char * tok0, char * tok1)
{
    uint8_t id;
    static struct Thing * t = NULL;
    if (t && (   parse_thing_type(tok0, tok1, t)
              || parse_thing_command(tok0, tok1, t)
              || parse_val(tok0, tok1, s[S_CMD_ARGUMENT], '8', (char *)&t->arg)
              || parse_val(tok0,tok1,s[S_CMD_PROGRESS],'8',(char *)&t->progress)
              || parse_val(tok0, tok1, s[S_CMD_LIFEPOINTS], '8',
                                                        (char *) &t->lifepoints)
              || parse_position(tok0, tok1, t)
              || parse_carry(tok0, tok1, t)));
    else if (parse_val(tok0, tok1, s[S_CMD_THING], '8', (char *) &id))
    {
        t = get_thing(world.things, id, 1);
        if (!t)
        {
            t = add_thing(id, 0, 0);
            set_cleanup_flag(CLEANUP_THINGS);
            t->fov_map= do_fov && t->lifepoints ? build_fov_map(t) : t->fov_map;
        }
    }
    else
    {
        return 0;
    }
    return 1;
}



static uint8_t parse_player_command(char * tok0, char * tok1)
{
    struct Thing * player = get_player();
    if (   parse_val(tok0, tok1, s[S_CMD_WAIT], '8', (char *) &player->arg)
        || parse_val(tok0, tok1, s[S_CMD_MOVE], '8', (char *) &player->arg)
        || parse_val(tok0, tok1, s[S_CMD_PICKUP], '8', (char *) &player->arg)
        || parse_val(tok0, tok1, s[S_CMD_DROP], '8', (char *) &player->arg)
        || parse_val(tok0, tok1, s[S_CMD_USE], '8', (char *) &player->arg))
    {
        player->command = get_thing_action_id_by_name(tok0);
        turn_over();
    }
    else
    {
        return 0;
    }
    return 1;
}



static void server_test()
{
    char * f_name = "server_test()";
    char test[10 + 1 + 10 + 1 + 1];
    FILE * file = try_fopen(s[S_PATH_OUT], "r", f_name);
    try_fgets(test, 10 + 10 + 1 + 1, file, f_name);
    try_fclose(file, f_name);
    if (strcmp(test, world.server_test))
    {
        unset_cleanup_flag(CLEANUP_WORLDSTATE);
        unset_cleanup_flag(CLEANUP_OUT);
        unset_cleanup_flag(CLEANUP_IN);
        char * msg = "Server test string in server output file does not match. "
                     "This indicates that the current server process has been "
                     "superseded by another one.";
        exit_err(1, msg);
    }
}



static void turn_over()
{
    struct Thing * player = get_player();
    struct Thing * thing = player;
    uint16_t start_turn = world.turn;
    while (    0 < player->lifepoints
           || (0 == player->lifepoints && start_turn == world.turn))
    {
        if (NULL == thing)
        {
            world.turn++;
            thing = world.things;
        }
        if (0 < thing->lifepoints)
        {
            if (0 == thing->command)
            {
                if (thing == player)
                {
                    break;
                }
                ai(thing);
            }
            thing->progress++;
            struct ThingAction * ta = world.thing_actions;
            while (ta->id != thing->command)
            {
                ta = ta->next;
            }
            if (thing->progress == ta->effort)
            {
                ta->func(thing);
                thing->command = 0;
                thing->progress = 0;
            }
        }
        thing = thing->next;
    }
}



static void record_msg(char * msg)
{
    char * f_name = "record_msg()";
    uint16_t size = strlen(s[S_PATH_RECORD]) + strlen(s[S_PATH_SUFFIX_TMP]) + 1;
    char * path_tmp = try_malloc(size, f_name);
    int test = sprintf(path_tmp, "%s%s", s[S_PATH_RECORD],s[S_PATH_SUFFIX_TMP]);
    exit_trouble(test < 0, f_name, "sprintf()");
    FILE * file_tmp  = try_fopen(path_tmp, "w", f_name);
    if (!access(s[S_PATH_RECORD], F_OK))
    {
        FILE * file_read = try_fopen(s[S_PATH_RECORD], "r", f_name);
        uint32_t linemax = textfile_width(file_read);
        char * line = try_malloc(linemax + 1, f_name);
        while (try_fgets(line, linemax + 1, file_read, f_name))
        {
            try_fwrite(line, strlen(line), 1, file_tmp, f_name);
        }
        free(line);
        try_fclose(file_read, f_name);
    }
    try_fwrite(msg, strlen(msg), 1, file_tmp, f_name);
    try_fputc('\n', file_tmp, f_name);
    try_fclose_unlink_rename(file_tmp, path_tmp, s[S_PATH_RECORD], f_name);
    free(path_tmp);
}



extern void obey_msg(char * msg, uint8_t do_record)
{
    set_err_line_options("Trouble with message: ", msg, 0, 0);
    char * msg_copy = strdup(msg);
    char * tok0 = token_from_line(msg_copy);
    char * tok1 = token_from_line(NULL);
    char * tok2 = token_from_line(NULL);
    if (err_line(!(tok0 && tok1) || tok2, "Bad number of tokens."))
    {
        return;
    }
    if (   parse_thing_manipulation(tok0, tok1)
        || parse_player_command(tok0, tok1)
        || parse_val(tok0, tok1, s[S_CMD_SEED_RAND], 'U', (char *) &world.seed)
        || parse_val(tok0, tok1, s[S_CMD_TURN], 'u', (char *) &world.turn)
        || parse_do_fov(tok0, tok1));
    else if (parse_val(tok0,tok1,s[S_CMD_SEED_MAP],'U',(char *)&world.seed_map))

    {
        remake_map();
    }
    else if (parse_val(tok0, tok1, s[S_CMD_MAKE_WORLD],'U',(char *)&world.seed))
    {
        remake_world();
    }
    else
    {
        err_line(1, "Unknown command.");
        free(msg_copy);
        return;
    }
    world.last_update_turn = 0;
    free(msg_copy);
    if (do_record)
    {
        save_world();
        record_msg(msg);
    }
}



extern uint8_t io_loop()
{
    char * f_name = "io_loop()";
    while (1)
    {
        server_test();
        char * msg = io_round();
        if (NULL == msg)
        {
            continue;
        }
        if (world.is_verbose)
        {
            exit_trouble(-1 == printf("Input: %s\n", msg), f_name, "printf()");
        }
        if (!strcmp("QUIT", msg))
        {
            free(msg);
            return 1;
        }
        if (!strcmp("PING", msg))
        {
            free(msg);
            char * pong = "PONG\n";
            try_fwrite(pong, strlen(pong), 1, world.file_out, f_name);
            fflush(world.file_out);
            continue;
        }
        if (world.replay)
        {
            free(msg);
            return 0;
        }
        obey_msg(msg, 1);
        free(msg);
    }
}
