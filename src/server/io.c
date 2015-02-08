/* src/server/io.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#define _POSIX_C_SOURCE 200112L /* snrpintf() */
#include "io.h"
#include <errno.h> /* global errno */
#include <limits.h> /* PIPE_BUF */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, int32_t, UINT8_MAX */
#include <stdio.h> /* defines FILE, sprintf(), fprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), snprintf(), memcpy(), strchr() */
#include <sys/types.h> /* time_t */
#include <time.h> /* time(), nanosleep() */
#include "../common/readwrite.h" /* atomic_write_start(), atomic_write_finish(),
                                  * get_message_from_queue(), try_fwrite(),
                                  * read_file_into_queue(), try_fputc()
                                  */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "hardcoded_strings.h" /* s */
#include "map.h" /* init_empty_map() */
#include "run.h" /* send_to_outfile() */
#include "things.h" /* Thing, ThingType, ThingInMemory, ThingAction,
                     * get_thing_type(), get_player()
                     */
#include "world.h" /* global world  */



/* Helpers to write lines of god commands to recreate thing "t". */
static void write_key_space(FILE * file, char * key);
static void write_uvalue(FILE * file, uint32_t value);
static void write_string(FILE * file, char * string);
static void write_key_space_uvalue(FILE * file, char * key, uint32_t value);
static void write_key_space_svalue(FILE * file, char * key, int32_t value);
static void write_key_space_string(FILE * file, char * key, char * string);

/* Write to "file" game-map-sized "map" in "command"-prefixed numbered lines. */
static void write_mem_map(FILE * file, char * map, char * command);

/* Write to "file" \n-delimited line of "key" + space + "value" as string. */
static void write_thing(FILE * file, struct Thing * t);

/* Poll input file for world.queue input. Wait a few seconds until giving up;
 * poll only every 0.03 seconds. Translate '\n' chars in input file into '\0'.
 */
static void try_growing_queue();

/* Write world state as visible to clients to its file. Write single dot line to
 * server output file to satisfy client ping mechanisms.
 */
static void update_worldstate_file();

/* Write "value" to new \n-delimited line of "file". */
static void write_value_as_line(int32_t value, FILE * file);

/* Write to "file" player's inventory, one item name per line. End in "%\n". */
static void write_inventory(struct Thing * player, FILE * file);

/* Return map cells sequence as visible to the "player", with invisible cells as
 * whitespace. Super-impose over visible map cells things positioned there,
 * with animate things overwriting inanimate things, and inanimate consumable
 * things overwriting inanimate non-consumable things.
 */
static char * build_visible_map(struct Thing * player);

/* Write to "file" game map as visible to "player" right now, as drawn by
 * build_visible_map(), and thereafter game map as memorized by player in its
 * .mem_map and .t_mem. Write one row per \n-delimited line.
 */
static void write_map(struct Thing * player, FILE * file);



static void write_key_space(FILE * file, char * key)
{
    try_fwrite(key, strlen(key), 1, file, __func__);
    try_fputc(' ', file, __func__);
}



static void write_uvalue(FILE * file, uint32_t value)
{
    char * line = try_malloc(11, __func__);
    exit_trouble(-1 == sprintf(line, "%u", value), __func__, s[S_FCN_SPRINTF]);
    try_fwrite(line, strlen(line), 1, file, __func__);
    free(line);
}



static void write_string(FILE * file, char * string)
{
    uint8_t contains_space = NULL != strchr(string, ' ');
    if (contains_space)
    {
        try_fputc('\'', file, __func__);
    }
    try_fwrite(string, strlen(string), 1, file, __func__);
    if (contains_space)
    {
        try_fputc('\'', file, __func__);
    }
}



static void write_key_space_uvalue(FILE * file, char * key, uint32_t value)
{
    write_key_space(file, key);
    write_uvalue(file, value);
    try_fputc('\n', file, __func__);
}



static void write_key_space_svalue(FILE * file, char * key, int32_t value)
{
    write_key_space(file, key);
    char * line = try_malloc(11, __func__);
    exit_trouble(-1 == sprintf(line, "%d", value), __func__, s[S_FCN_SPRINTF]);
    try_fwrite(line, strlen(line), 1, file, __func__);
    free(line);
    try_fputc('\n', file, __func__);
}



static void write_key_space_string(FILE * file, char * key, char * string)
{
    write_key_space(file, key);
    write_string(file, string);
    try_fputc('\n', file, __func__);
}



static void write_mem_map(FILE * file, char * map, char * command)
{
    if (map)
    {
        uint32_t map_size = world.map.length * world.map.length;/* snprintf() */
        char * map_copy = try_malloc(map_size + 1, __func__);    /* reads one */
        memcpy(map_copy, map, map_size);              /* byte beyond map_size */
        map_copy[map_size] = '\0';         /* if string is not \0-terminated. */
        uint16_t y;
        char string[UINT8_MAX + 1 + 1];
        for (y = 0; y < world.map.length; y++)
        {
            int test = snprintf(string, world.map.length + 1, "%s",
                                map_copy + (y * world.map.length));
            exit_trouble(test < 0, __func__, "snprintf()");
            write_key_space(file, command);
            write_uvalue(file, y);
            try_fputc(' ', file, __func__);
            write_string(file, string);
            try_fputc('\n', file, __func__);
        }
        free(map_copy);
    }
}



static void write_thing(FILE * file, struct Thing * t)
{
    struct Thing * o;
    for (o = t->owns; o; o = o->next)
    {
        write_thing(file, o);
    }
    write_key_space_uvalue(file, s[S_CMD_T_ID], t->id);
    write_key_space_uvalue(file, s[S_CMD_T_TYPE], t->type);
    write_key_space_uvalue(file, s[S_CMD_T_POSY], t->pos.y);
    write_key_space_uvalue(file, s[S_CMD_T_POSX], t->pos.x);
    write_key_space_uvalue(file, s[S_CMD_T_COMMAND], t->command);
    write_key_space_uvalue(file, s[S_CMD_T_ARGUMENT], t->arg);
    write_key_space_uvalue(file, s[S_CMD_T_PROGRESS], t->progress);
    write_key_space_uvalue(file, s[S_CMD_T_HP], t->lifepoints);
    write_key_space_svalue(file, s[S_CMD_T_SATIATION], t->satiation);
    for (o = t->owns; o; o = o->next)
    {
        write_key_space_uvalue(file, s[S_CMD_T_CARRIES], o->id);
    }
    write_mem_map(file, t->mem_depth_map, s[S_CMD_T_MEMDEPTHMAP]);
    write_mem_map(file, t->mem_map, s[S_CMD_T_MEMMAP]);
    struct ThingInMemory * tm = t->t_mem;
    for (; tm; tm = tm->next)
    {
        write_key_space(file, s[S_CMD_T_MEMTHING]);
        write_uvalue(file, tm->type);
        try_fputc(' ', file, __func__);
        write_uvalue(file, tm->pos.y);
        try_fputc(' ', file, __func__);
        write_uvalue(file, tm->pos.x);
        try_fputc('\n', file, __func__);
    }
    try_fputc('\n', file, __func__);
}



static void try_growing_queue()
{
    uint8_t wait_seconds = 5;
    time_t now = time(0);
    struct timespec dur;
    dur.tv_sec = 0;
    dur.tv_nsec = 33333333;
    while (1)
    {
        if (read_file_into_queue(world.file_in, &world.queue))
        {
            return;
        }
        nanosleep(&dur, NULL);
        if (time(0) > now + wait_seconds)
        {
            return;
        }
    }
}



static void update_worldstate_file()
{
    char * path_tmp;
    FILE * file = atomic_write_start(s[S_PATH_WORLDSTATE], &path_tmp);
    struct Thing * player = get_player();
    write_value_as_line(world.turn, file);
    write_value_as_line(player->lifepoints, file);
    write_value_as_line(player->satiation, file);
    write_inventory(player, file);
    write_value_as_line(player->pos.y, file);
    write_value_as_line(player->pos.x, file);
    write_value_as_line(world.map.length, file);
    write_map(player, file);
    atomic_write_finish(file, s[S_PATH_WORLDSTATE], path_tmp);
    set_cleanup_flag(CLEANUP_WORLDSTATE);
    char * dot = ".\n";
    try_fwrite(dot, strlen(dot), 1, world.file_out, __func__);
    fflush(world.file_out);
}



static void write_value_as_line(int32_t value, FILE * file)
{
    char write_buf[13]; /* Hold "+"/"-" + 10 digits of int32_t max + \n + \0. */
    exit_trouble(sprintf(write_buf,"%u\n",value) < 0,__func__,s[S_FCN_SPRINTF]);
    try_fwrite(write_buf, strlen(write_buf), 1, file, __func__);
}



static void write_inventory(struct Thing * player, FILE * file)
{
    struct Thing * owned = player->owns;
    if (!owned)
    {
        char * empty = "(none)\n";
        try_fwrite(empty, strlen(empty), 1, file, __func__);
    }
    else
    {
        uint8_t q;
        for (q = 0; owned; q++)
        {
            struct ThingType * tt = get_thing_type(owned->type);
            try_fwrite(tt->name, strlen(tt->name), 1, file, __func__);
            try_fputc('\n', file, __func__);
            owned = owned->next;
        }
    }
    try_fputc('%', file, __func__);
    try_fputc('\n', file, __func__);
}



static char * build_visible_map(struct Thing * player)
{
    char * visible_map;
    init_empty_map(&visible_map);
    if (player->fov_map) /* May fail if player thing was created / positioned */
    {                    /* by god command after turning off FOV building.    */
        uint32_t pos_i = 0;
        for (; pos_i < (uint32_t) world.map.length * world.map.length; pos_i++)
        {
            if (player->fov_map[pos_i] == 'v')
            {
                visible_map[pos_i] = world.map.cells[pos_i];
            }
        }
        struct Thing * t;
        char c;
        uint8_t i;
        for (i = 0; i < 3; i++)
        {
            for (t = world.things; t != 0; t = t->next)
            {
                if ('v' == player->fov_map[t->pos.y*world.map.length+t->pos.x])
                {
                    struct ThingType * tt = get_thing_type(t->type);
                    if (   (0 == i && !t->lifepoints && !tt->consumable)
                        || (1 == i && !t->lifepoints &&  tt->consumable)
                        || (2 == i &&  t->lifepoints))
                    {
                        c = tt->char_on_map;
                        visible_map[t->pos.y * world.map.length + t->pos.x] = c;
                    }
                }
            }
        }
    }
    return visible_map;
}



static void write_map(struct Thing * player, FILE * file)
{
    char * visible_map = build_visible_map(player);
    uint16_t x, y;
    for (y = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            try_fputc(visible_map[y * world.map.length + x], file, __func__);
        }
        try_fputc('\n', file, __func__);
    }
    free(visible_map);
    uint32_t map_size = world.map.length * world.map.length;
    char * mem_map = try_malloc(map_size, __func__);
    memcpy(mem_map, player->mem_map, map_size);
    uint8_t i;
    struct ThingInMemory * tm;
    for (i = 0; i < 2; i++)
    {
        for (tm = player->t_mem; tm; tm = tm->next)
        {
            if (' ' != player->mem_map[tm->pos.y*world.map.length+tm->pos.x])
            {
                struct ThingType * tt = get_thing_type(tm->type);
                if (   (0 == i && !tt->consumable)
                    || (1 == i &&  tt->consumable))
                {
                    char c = tt->char_on_map;
                    mem_map[tm->pos.y * world.map.length + tm->pos.x] = c;
                }
            }
        }
    }
    for (y = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            try_fputc(mem_map[y * world.map.length + x], file, __func__);
        }
        try_fputc('\n', file, __func__);
    }
    free(mem_map);
}



extern char * io_round()
{
    if (world.queue && strlen(world.queue))
    {
        return get_message_from_queue(&world.queue);
    }
    if (world.do_update)
    {
        update_worldstate_file();
        send_to_outfile("WORLD_UPDATED\n", 1);
        world.do_update = 0;
    }
    try_growing_queue();
    return get_message_from_queue(&world.queue);
}



extern void save_world()
{
    char * path_tmp;
    FILE * file = atomic_write_start(s[S_PATH_SAVE], &path_tmp);
    write_key_space_uvalue(file, s[S_CMD_MAPLENGTH], world.map.length);
    write_key_space_uvalue(file, s[S_CMD_PLAYTYPE], world.player_type);
    try_fputc('\n', file, __func__);
    struct ThingAction * ta;
    for (ta = world.thing_actions; ta; ta = ta->next)
    {
        write_key_space_uvalue(file, s[S_CMD_TA_ID], ta->id);
        write_key_space_uvalue(file, s[S_CMD_TA_EFFORT], ta->effort);
        write_key_space_string(file, s[S_CMD_TA_NAME], ta->name);
        try_fputc('\n', file, __func__);
    }
    struct ThingType * tt;
    for (tt = world.thing_types; tt; tt = tt->next)
    {
        write_key_space_uvalue(file, s[S_CMD_TT_ID], tt->id);
        write_key_space_uvalue(file, s[S_CMD_TT_STARTN], tt->start_n);
        write_key_space_uvalue(file, s[S_CMD_TT_HP], tt->lifepoints);
        int test = fprintf(file, "%s %c\n", s[S_CMD_TT_SYMB], tt->char_on_map);
        exit_trouble(test < 0, __func__, "fprintf");
        write_key_space_string(file, s[S_CMD_TT_NAME], tt->name);
        write_key_space_uvalue(file, s[S_CMD_TT_CONSUM], tt->consumable);
        write_key_space_uvalue(file, s[S_CMD_TT_PROL], tt->proliferate);
        write_key_space_uvalue(file, s[S_CMD_TT_STOMACH], tt->stomach);
        try_fputc('\n', file, __func__);
    }
    for (tt = world.thing_types; tt; tt = tt->next)
    {
        write_key_space_uvalue(file, s[S_CMD_TT_ID], tt->id);
        write_key_space_uvalue(file, s[S_CMD_TT_CORPS], tt->corpse_id);
    }
    try_fputc('\n', file, __func__);
    write_key_space_uvalue(file, s[S_CMD_SEED_MAP], world.seed_map);
    write_key_space_uvalue(file, s[S_CMD_SEED_RAND], world.seed);
    write_key_space_uvalue(file, s[S_CMD_TURN], world.turn);
    try_fputc('\n', file, __func__);
    struct Thing * t;
    for (t = world.things; t; t = t->next)
    {
        write_thing(file, t);
    }
    write_key_space_uvalue(file, s[S_CMD_WORLD_ACTIVE], 1);
    atomic_write_finish(file, s[S_PATH_SAVE], path_tmp);
}
