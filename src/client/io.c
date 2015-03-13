/* src/client/io.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#define _POSIX_C_SOURCE 1 /* PIPE_BUF */
#include "io.h"
#include <limits.h> /* PIPE_BUF */
#include <ncurses.h> /* timeout(), getch() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT32_MAX */
#include <stdio.h> /* FILE, sprintf(), fseek(), fflush() */
#include <string.h> /* strcmp(), strlen(), memcpy() */
#include <stdlib.h> /* free(), atoi() */
#include <sys/stat.h> /* stat() */
#include <sys/types.h> /* time_t */
#include <time.h> /* time() */
#include <unistd.h> /* access() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/rexit.h" /* exit_trouble(), exit_err() */
#include "../common/readwrite.h" /* try_fopen(), try_fclose(), try_fgets(),
                                  * try_fgetc(), textfile_width(), try_fputc(),
                                  * read_file_into_queue(),
                                  * get_message_from_queue(),
                                  */
#include "../common/yx_uint8.h" /* yx_uint8 */
#include "control.h" /* try_key() */
#include "map.h" /* query_mapcell() */
#include "windows.h" /* Win, reset_windows_on_winch(), draw_all_wins(),
                      * get_win_by_id()
                      */
#include "world.h" /* world global */



/* Read next lines of "file" up to (and excluding) a line "%\n" into the
 * world.player_inventory string.
 */
static void read_inventory(char * read_buf, uint32_t linemax, FILE * file);

/* Read the next characters in "file" into "map". In detail: Read
 * world.map.length times world.map.length characters, followed by one ignored
 * character (that we assume is a newline).
 */
static void read_map_cells(FILE * file, char ** map);

/* Return value seen by atoi() in next line of "file" when passed to try_fgets()
 * with the given arguments.
 */
static int32_t read_value_from_line(char* read_buf,int32_t linemax,FILE* file);

/* If the server's worldstate file has changed since the last read_worldstate(),
 * return a pointer to its file descriptor; else, return NULL.
 *
 * Two tests are performed to check for a file change. The file's last data
 * modification time in seconds via stat() is compared against world.last_update
 * (and if it is changed, world.last_update is re-set to it). If this does not
 * verify a change, the first bytes of the file are read to compare the game
 * turn number described therein to the last read turn number in world.turn.
 *
 * The stat() check is mostly useless, for it only detects file updates once a
 * second. But the turn check fails if a new world is generated from turn 1 on:
 * the new world also starts in turn 1, not signifying any world change to the
 * turn check. The stat() check detects this change with at most 1 second delay.
 */
static FILE * changed_worldstate_file(char * path);

/* Attempt to read the server's worldstate file as representation of the game
 * world in a hard-coded serialization format. Returns 1 on success, or 0 if the
 * out file wasn't read for supposedly not having changed since a last
 * read_worldstate() call.
 */
static uint8_t read_worldstate();

/* Poll server for queue input. If no new input, send ping, or, if ping already
 * sent but unanswered for some time, abort.
 */
static void test_and_poll_server();

/* If "string", append \n-prefixed "append", else write "append" as "string". */
static void nl_append_string(char * append, char ** string);

/* Read messages from queue, act on them. */
static uint8_t read_queue();



static void read_inventory(char * read_buf, uint32_t linemax, FILE * file)
{
    char * delimiter = "%\n";
    free(world.player_inventory);
    world.player_inventory = NULL;          /* Avoids illegal strlen() below. */
    while (1)
    {
        try_fgets(read_buf, linemax + 1, file, __func__);
        if (!(strcmp(read_buf, delimiter)))
        {
            break;
        }
        int old_size = 0;
        if (world.player_inventory)
        {
            old_size = strlen(world.player_inventory);
        }
        int new_size = strlen(read_buf);
        char * new_inventory = try_malloc(old_size + new_size + 1, __func__);
        memcpy(new_inventory, world.player_inventory, old_size);
        int test = sprintf(new_inventory + old_size, "%s", read_buf);
        exit_trouble(test < 0, __func__, "sprintf");
        free(world.player_inventory);
        world.player_inventory = new_inventory;
    }
    world.player_inventory[strlen(world.player_inventory) - 1] = '\0';
    world.player_inventory_select = 0;
}



static void read_map_cells(FILE * file, char ** map)
{
    if (*map)
    {
        free(*map);
        *map = NULL;
    }
    *map = try_malloc(world.map.length * world.map.length, __func__);
    char * map_cells = *map;
    uint16_t y, x;
    for (y = 0; y < world.map.length; y++)
    {
        for (x = 0; x < world.map.length; x++)
        {
            char c = try_fgetc(file, __func__);
            map_cells[y * world.map.length + x] = c;
        }
        try_fgetc(file, __func__);
    }
}



static int32_t read_value_from_line(char * read_buf,int32_t linemax,FILE * file)
{
    try_fgets(read_buf, linemax + 1, file, __func__);
    return atoi(read_buf);
}



static FILE * changed_worldstate_file(char * path)
{
    struct stat stat_buf;
    exit_trouble(stat(path, &stat_buf), __func__, "stat");
    if (stat_buf.st_mtime != world.last_update)
    {
        world.last_update = stat_buf.st_mtime;
        return try_fopen(path, "r", __func__);
    }
    FILE * file = try_fopen(path, "r", __func__);
    char turn_string[6];
    try_fgets(turn_string, 6, file, __func__);
    if (world.turn == atoi(turn_string))
    {
        try_fclose(file, __func__);
        return NULL;
    }
    exit_trouble(fseek(file, 0, SEEK_SET), __func__, "fseek");
    return file;
}



static uint8_t read_worldstate()
{
    char * path = "server/worldstate";
    char * quit_msg = "No worldstate file found to read. Server may be down.";
    exit_err(access(path, F_OK), quit_msg);
    FILE * file = changed_worldstate_file(path);
    if (!file)
    {
        return 0;
    }
    uint32_t linemax = textfile_width(file);
    char * read_buf = try_malloc(linemax + 1, __func__);
    world.turn = (uint16_t) read_value_from_line(read_buf, linemax, file);
    world.godsmood = (int16_t) read_value_from_line(read_buf, linemax, file); //
    world.godsfavor = (int16_t) read_value_from_line(read_buf, linemax, file); //
    world.player_lifepoints = (uint16_t) read_value_from_line(read_buf, linemax,
                                                              file);
    world.player_satiation = (int16_t) read_value_from_line(read_buf, linemax,
                                                            file);
    read_inventory(read_buf, linemax, file);
    world.player_pos.y = (uint8_t) read_value_from_line(read_buf,linemax,file);
    world.player_pos.x = (uint8_t) read_value_from_line(read_buf,linemax,file);
    world.map.length = (uint16_t) read_value_from_line(read_buf, linemax, file);
    read_map_cells(file, &world.map.cells);
    read_map_cells(file, &world.mem_map);
    read_map_cells(file, &world.stacks_map);  //
    free(read_buf);
    try_fclose(file, __func__);
    return 1;
}



static void test_and_poll_server()
{
    static time_t last_server_answer_time = 0;
    static time_t last_pong_time = 0;
    static uint8_t ping_sent = 0;
    if (read_file_into_queue(world.file_server_out, &world.queue))
    {
        last_server_answer_time = time(0);
        return;
    }
    time_t now = time(0);
    if (ping_sent && last_server_answer_time > now - 5)  /* Re-set if last    */
    {                                                    /* ping was answered */
        ping_sent = 0;                                   /* with server       */
        return;                                          /* activity.         */
    }
    if (!ping_sent && last_server_answer_time < now - 5)
    {
        send("PING");
        ping_sent = 1;
        last_pong_time = now;
        return;
    }
    exit_err(ping_sent && last_pong_time < now - 5, "Server not answering.");
}



static void nl_append_string(char * append, char ** string)
{
    char * err = "too large sizes";
    exit_trouble(UINT32_MAX < strlen(append), __func__, err);
    uint32_t new_size = strlen(append);
    uint32_t old_size = 0;
    uint8_t add_nl = 0;
    if (*string)
    {
        exit_trouble(UINT32_MAX < new_size + strlen(*string), __func__, err);
        old_size = strlen(*string);
        add_nl = 1;
    }
    char * new_string = try_malloc(old_size + add_nl + new_size + 1, __func__);
    memcpy(new_string, *string, old_size);
    char * pattern = add_nl ? "\n%s" : "%s";
    int test = sprintf(new_string + old_size, pattern, append);
    exit_trouble(test < 0, __func__, "sprintf");
    free(*string);
    *string = new_string;
}



static uint8_t read_queue()
{
    static uint8_t things_here_parsing = 0;
    uint8_t ret = 0;
    char * msg;
    while (NULL != (msg = get_message_from_queue(&world.queue)))
    {
        char * log_prefix = "LOG ";
        if      (!strcmp(msg, "THINGS_HERE START"))
        {
            ret = 1;
            things_here_parsing = 1;
            free(world.things_here);
            world.things_here = NULL;
        }
        else if (!strcmp(msg, "THINGS_HERE END"))
        {
            things_here_parsing = 0;
            if (!world.things_here)
            {
                nl_append_string("(none known)", &world.things_here);
            }
            world.things_here_scroll = 0;  //
        }
        else if (things_here_parsing)
        {
            ret = 1;
            nl_append_string(msg, &world.things_here);
        }
        else if (!strncmp(msg, log_prefix, strlen(log_prefix)))
        {
            ret = 1;
            nl_append_string(msg + strlen(log_prefix), &world.log);
        }
        else if (!strcmp(msg, "NEW_WORLD"))
        {
            ret = 1;
            free(world.log);
            world.log = NULL;
        }
        else if (!strcmp(msg, "WORLD_UPDATED"))
        {
            query_mapcell();
        }
        free(msg);
    }
    return ret;
}



extern void send(char * msg)
{
    uint32_t msg_size = strlen(msg) + 1;
    char * err = "send() tried to send message larger than PIPE_BUF bytes.";
    exit_err(msg_size > PIPE_BUF, err);
    try_fwrite(msg, strlen(msg), 1, world.file_server_in, __func__);
    try_fputc('\n', world.file_server_in, __func__);
    fflush(world.file_server_in);
}



extern char * io_loop()
{
    uint16_t delay = 1; /* Greater delay: less redundant server files reading.*/
    uint8_t change_in_client = 0;
    while (1)
    {
        timeout(delay);
        delay = delay < 1000 ? delay * 2 : delay;
        test_and_poll_server();
        if (world.winch)
        {
            reset_windows_on_winch();
            world.winch = 0;
            change_in_client++;
        }
        if (change_in_client || read_worldstate() || read_queue())
        {
            delay = 1;       /* Keep client alert even if it's not getch()'d. */
            struct Win * win_map = get_win_by_id('m');
            if (0 == win_map->view)   /* So the map window's winconfig views  */
            {                         /* don't get confused by the centering. */
                struct yx_uint8 pos=world.look?world.look_pos:world.player_pos;
                win_map->center.y = pos.y;
                win_map->center.x = pos.x * 2 + (pos.y % 2);
            }
            draw_all_wins();
        }
        change_in_client = 0;
        int key = getch();
        if (ERR != key)
        {
            delay = 1;                     /* Alert client if it's getch()'d. */
            change_in_client = try_key((uint16_t) key);
            if (2 == change_in_client)
            {
                break;
            }
        }
    }
    send("QUIT");
    return "Sent QUIT to server.";
}
