/* src/server/io.c */

#define _POSIX_C_SOURCE 199309L
#include "io.h"
#include <errno.h> /* global errno */
#include <limits.h> /* PIPE_BUF */
#include <stddef.h> /* size_t, NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* defines EOF, FILE, sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), memcpy(), memset() */
#include <sys/types.h> /* time_t */
#include <time.h> /* time(), nanosleep() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose_unlink_rename(),
                                  * try_fwrite(), try_fputc(), try_fgetc()
                                  */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "field_of_view.h" /* VISIBLE */
#include "map.h" /* yx_to_map_pos() */
#include "things.h" /* Thing, ThingType, get_thing_type(), get_player() */
#include "world.h" /* global world  */



/* Cut out and return first \0-terminated string from world.queue and
 * appropriately reduce world.queue_size. Return NULL if queue is empty.
 * Superfluous \0 bytes after the string are also cut out. Should the queue
 * start with \0 bytes, those are cut out, but NULL is returned instead of "".
*/
static char * get_message_from_queue();

/* Poll input file for world.queue input. Wait a few seconds until giving up;
 * poll only every 0.03 seconds.. Translate '\n' chars in input file into '\0'.
 */
static void read_file_into_queue();

/* Write world state as visible to clients to its file. Write single dot line to
 * server output file to satisfy client ping mechanisms.
 */
static void update_worldstate_file();

/* Write "value" to new \n-delimited line of "file". */
static void write_value_as_line(uint32_t value, FILE * file);

/* Write to "file" player's inventory, one item name per line. End in "%\n". */
static void write_inventory(struct Thing * player, FILE * file);

/* Return map cells sequence as visible to the "player", with invisible cells as
 * whitespace. Super-impose over visible map cells things positioned there.
 */
static char * build_visible_map(struct Thing * player);

/* Write to "file" game map as visible to "player", build_visible_map()-drawn.
 * Write one row per \n-delimited line.
 */
static void write_map(struct Thing * player, FILE * file);



static char * get_message_from_queue()
{
    char * f_name = "get_message_from_queue()";
    char * message = NULL;
    if (world.queue_size)
    {
        size_t cutout_len = strlen(world.queue);
        if (0 < cutout_len)
        {
            cutout_len++;
            message = try_malloc(cutout_len, f_name);
            memcpy(message, world.queue, cutout_len);
        }
        for (;
             cutout_len != world.queue_size && '\0' == world.queue[cutout_len];
             cutout_len++);
        world.queue_size = world.queue_size - cutout_len;
        if (0 == world.queue_size)
        {
            free(world.queue);   /* NULL so read_file_into_queue() may free() */
            world.queue = NULL;  /* this every time, even when it's           */
        }                        /* un-allocated first. */
        else
        {
            char * new_queue = try_malloc(world.queue_size, f_name);
            memcpy(new_queue, &(world.queue[cutout_len]), world.queue_size);
            free(world.queue);
            world.queue = new_queue;
        }
    }
    return message;
}



static void read_file_into_queue()
{
    char * f_name = "read_file_into_queue()";
    uint8_t wait_seconds = 5;
    time_t now = time(0);
    struct timespec dur;
    dur.tv_sec = 0;
    dur.tv_nsec = 33333333;
    int test;
    while (EOF == (test = try_fgetc(world.file_in, f_name)))
    {
        nanosleep(&dur, NULL);
        if (time(0) > now + wait_seconds)
        {
            return;
        }
    }
    do
    {
        char c = (char) test;
        if ('\n' == c)
        {
            c = '\0';
        }
        char * new_queue = try_malloc(world.queue_size + 1, f_name);
        memcpy(new_queue, world.queue, world.queue_size);
        char * new_pos = new_queue + world.queue_size;
        * new_pos = c;
        world.queue_size++;
        free(world.queue);
        world.queue = new_queue;
    }
    while (EOF != (test = try_fgetc(world.file_in, f_name)));
}



static void update_worldstate_file()
{
    char * f_name = "update_worldstate_file()";
    char path_tmp[strlen(world.path_worldstate) + strlen(world.tmp_suffix) + 1];
    sprintf(path_tmp, "%s%s", world.path_worldstate, world.tmp_suffix);
    FILE * file = try_fopen(path_tmp, "w", f_name);
    struct Thing * player = get_player();
    write_value_as_line(world.turn, file);
    write_value_as_line(player->lifepoints, file);
    write_inventory(player, file);
    write_value_as_line(player->pos.y, file);
    write_value_as_line(player->pos.x, file);
    write_value_as_line(world.map.length, file);
    write_map(player, file);
    if (world.log)
    {
        try_fwrite(world.log, strlen(world.log), 1, file, f_name);
    }
    try_fclose_unlink_rename(file, path_tmp, world.path_worldstate, f_name);
    set_cleanup_flag(CLEANUP_WORLDSTATE);
    char * dot = ".\n";;
    try_fwrite(dot, strlen(dot), 1, world.file_out, f_name);
    fflush(world.file_out);
}



static void write_value_as_line(uint32_t value, FILE * file)
{
    char * f_name = "write_value_as_line()";
    char write_buf[12];     /* Holds 10 digits of uint32_t maximum + \n + \0. */
    sprintf(write_buf, "%u\n", value);
    try_fwrite(write_buf, strlen(write_buf), 1, file, f_name);
}



static void write_inventory(struct Thing * player, FILE * file)
{
    char * f_name = "write_inventory()";
    struct Thing * owned = player->owns;
    if (NULL == owned)
    {
        char * empty = "(none)\n";
        try_fwrite(empty, strlen(empty), 1, file, f_name);
    }
    else
    {
        uint8_t q;
        for (q = 0; NULL != owned; q++)
        {
            struct ThingType * tt = get_thing_type(owned->type);
            try_fwrite(tt->name, strlen(tt->name), 1, file, f_name);
            try_fputc('\n', file, f_name);
            owned = owned->next;
        }
    }
    try_fputc('%', file, f_name);
    try_fputc('\n', file, f_name);
}



static char * build_visible_map(struct Thing * player)
{
    char * f_name = "build_visible_map()";
    uint32_t map_size = world.map.length * world.map.length;
    char * visible_map = try_malloc(map_size, f_name);
    memset(visible_map, ' ', map_size);
    uint16_t pos_i;
    for (pos_i = 0; pos_i < map_size; pos_i++)
    {
        if (player->fov_map[pos_i] & VISIBLE)
        {
            visible_map[pos_i] = world.map.cells[pos_i];
        }
    }
    struct Thing * t;
    struct ThingType * tt;
    char c;
    uint8_t i;
    for (i = 0; i < 2; i++)
    {
        for (t = world.things; t != 0; t = t->next)
        {
            if (   player->fov_map[yx_to_map_pos(&t->pos)] & VISIBLE
                && (   (0 == i && 0 == t->lifepoints)
                    || (1 == i && 0 < t->lifepoints)))
            {
                tt = get_thing_type(t->type);
                c = tt->char_on_map;
                visible_map[yx_to_map_pos(&t->pos)] = c;
            }
        }
    }
    return visible_map;
}



static void write_map(struct Thing * player, FILE * file)
{
    char * f_name = "write_map()";
    char * visible_map = build_visible_map(player);
    uint16_t x, y;
    for (y = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            try_fputc(visible_map[(y * world.map.length) + x], file, f_name);
        }
        try_fputc('\n', file, f_name);
    }
    free(visible_map);
}



extern char * io_round()
{
    char * f_name = "io_round()";
    if (0 < world.queue_size)
    {
        return get_message_from_queue();
    }
    if (world.turn != world.last_update_turn)
    {
        update_worldstate_file();
        world.last_update_turn = world.turn;
    }
    read_file_into_queue();
    if (world.queue_size && '\0' != world.queue[world.queue_size - 1])
    {
        char * new_queue = try_malloc(world.queue_size + 1, f_name);
        memcpy(new_queue, world.queue, world.queue_size);
        new_queue[world.queue_size] = '\0';
        world.queue_size++;
        free(world.queue);
        world.queue = new_queue;
    }
    return get_message_from_queue();
}
