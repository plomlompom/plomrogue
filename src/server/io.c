/* src/server/io.c */

#include "io.h"
#include <errno.h> /* global errno */
#include <fcntl.h> /* open(), O_RDONLY, O_NONBLOCK */
#include <limits.h> /* PIPE_BUF */
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* define FILE, sprintf() */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), memset(), memcpy() */
#include <unistd.h> /* read(), close() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose_unlink_rename(),
                                  * try_fwrite(), try_fputc()
                                  */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "map_objects.h" /* structs MapObj, MapObjDef, get_map_obj_def() */
#include "world.h" /* global world  */



/* Cut out and return first \0-terminated string from world.queue and
 * appropriately reduce world.queue_size. Return NULL if queue is empty.
 * Superfluous \0 bytes after the string are also cut out. Should the queue
 * start with \0 bytes, those are cut out, but NULL is returned instead of "".
*/
static char * get_message_from_queue();

/* Read fifo input to put into queue. Only stop reading when bytes were received
 * and the receiving has stopped. Read max. PIPE_BUF-sized chunks for atomicity.
 */
static void read_fifo_into_queue();

/* Write to output file the world state as it is to be visible to clients. */
static void update_out_file();

/* Write "value" to new \n-delimited line of "file". */
static void write_value_as_line(uint32_t value, FILE * file);

/* Write to "file" player's inventory, one item name per line. End in "%\n". */
static void write_inventory(struct MapObj * player, FILE * file);

/* Write to "file" game map, with map objects super-imposed. Write one row per
 * \n-delimited line. Super-impose animated objects over inanimate objects.
 */
static void write_map(FILE * file);



static char * get_message_from_queue()
{
    char * f_name = "get_message_from_queue()";
    char * message = NULL;
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
        free(world.queue);
        world.queue = NULL;
    }
    else
    {
        char * new_queue = try_malloc(world.queue_size, f_name);
        memcpy(new_queue, &(world.queue[cutout_len]), world.queue_size);
        free(world.queue);
        world.queue = new_queue;
    }
    return message;
}



static void read_fifo_into_queue()
{
    char * f_name = "read_fifo_into_queue()";
    uint32_t buf_size = PIPE_BUF;
    int fdesc_infile = open(world.path_in, O_RDONLY | O_NONBLOCK);
    exit_trouble(-1 == fdesc_infile, "open()", f_name);
    char buf[buf_size];
    memset(buf, 0, buf_size);
    int bytes_read;
    uint8_t read_state = 0; /* 0: waiting for input. 1: started receiving it. */
    while (1)
    {
        bytes_read = read(fdesc_infile, buf, buf_size);
        if (bytes_read > 0)
        {
            read_state = 1;
            uint32_t old_queue_size = world.queue_size;
            world.queue_size = world.queue_size + bytes_read;
            char * new_queue = try_malloc(world.queue_size, f_name);
            memcpy(new_queue, world.queue, old_queue_size);
            memcpy(&(new_queue[old_queue_size]), buf, bytes_read);
            free(world.queue);
            world.queue = new_queue;
            memset(buf, 0, buf_size);
            continue;
        }
        exit_trouble(-1 == bytes_read && errno != EAGAIN, "read()", f_name);
        if (1 == read_state)
        {
            break;
        }
    }
    exit_trouble(close(fdesc_infile), f_name, "close()");
}



static void update_out_file()
{
    char * f_name = "update_out_file()";
    char path_tmp[strlen(world.path_out) + strlen(world.tmp_suffix) + 1];
    sprintf(path_tmp, "%s%s", world.path_out, world.tmp_suffix);
    FILE * file = try_fopen(path_tmp, "w", f_name);
    struct MapObj * player = get_player();
    write_value_as_line(world.turn, file);
    write_value_as_line(world.score, file);
    write_value_as_line(player->lifepoints, file);
    write_inventory(player, file);
    write_value_as_line(player->pos.y, file);
    write_value_as_line(player->pos.x, file);
    write_value_as_line(world.map.size.y, file);
    write_value_as_line(world.map.size.x, file);
    write_map(file);
    if (world.log)
    {
        try_fwrite(world.log, strlen(world.log), 1, file, f_name);
    }
    try_fclose_unlink_rename(file, path_tmp, world.path_out, f_name);
    set_cleanup_flag(CLEANUP_OUTFILE);
}



static void write_value_as_line(uint32_t value, FILE * file)
{
    char * f_name = "write_value_as_line()";
    char write_buf[12];     /* Holds 10 digits of uint32_t maximum + \n + \0. */
    sprintf(write_buf, "%u\n", value);
    try_fwrite(write_buf, strlen(write_buf), 1, file, f_name);
}



static void write_inventory(struct MapObj * player, FILE * file)
{
    char * f_name = "write_inventory()";
    struct MapObj * owned = player->owns;
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
            struct MapObjDef * mod = get_map_object_def(owned->type);
            try_fwrite(mod->name, strlen(mod->name), 1, file, f_name);
            try_fputc('\n', file, f_name);
            owned = owned->next;
        }
    }
    try_fputc('%', file, f_name);
    try_fputc('\n', file, f_name);
}



static void write_map(FILE * file)
{
    char * f_name = "write_map()";
    uint32_t map_size = world.map.size.y * world.map.size.x;
    char visible_map[map_size];
    memcpy(visible_map, world.map.cells, map_size);
    struct MapObj * o;
    struct MapObjDef * d;
    char c;
    uint8_t i;
    for (i = 0; i < 2; i++)
    {
        for (o = world.map_objs; o != 0; o = o->next)
        {
            if ((   (0 == i && 0 == o->lifepoints)
                 || (1 == i && 0 < o->lifepoints)))
            {
                d = get_map_object_def(o->type);
                c = d->char_on_map;
                visible_map[(o->pos.y * world.map.size.x) + o->pos.x] = c;
            }
        }
    }
    uint16_t x, y;
    for (y = 0; y < world.map.size.y; y++)
    {
        for (x = 0; x < world.map.size.x; x++)
        {
            try_fputc(visible_map[(y * world.map.size.x) + x], file, f_name);
        }
        try_fputc('\n', file, f_name);
    }
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
        update_out_file();
        world.last_update_turn = world.turn;
    }
    read_fifo_into_queue();
    if ('\0' != world.queue[world.queue_size - 1])
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
