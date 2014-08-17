/* src/client/io.c */

#define _POSIX_C_SOURCE 1 /* PIPE_BUF */
#include "io.h"
#include <limits.h> /* PIPE_BUF */
#include <ncurses.h> /* halfdelay(), getch() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
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
                                  * try_fgetc(), textfile_width(), try_fputc()
                                  */
#include "control.h" /* try_key() */
#include "map.h" /* map_center() */
#include "windows.h" /* reset_windows_on_winch(), draw_all_wins() */
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

/* Repeatedly use try_fgets() with given arguments to read the remaining lines
 * of "file" into the world.log string.
 */
static void read_log(char * read_buf, uint32_t linemax, FILE * file);

/* Return value seen by atoi() in next line of "file" when passed to try_fgets()
 * with the given arguments.
 */
static uint16_t read_value_from_line(char * read_buf, uint32_t linemax,
                                     FILE * file);

/* If the server's worldstate file has changed since the last read_world(),
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
 * world in a hard-coded serialization format. Returns 1 on success and 0 if the
 * out file wasn't read for supposedly not having changed since a last
 * read_world() call.
 *
 * map_center() is triggered by either, the first successful read_world() (thus
 * on client start), or on turn 1 (thus on world start).
 */
static uint8_t read_world();

/* If "last_server_answer_time" is too old, send a PING to the server; or, if a
 * previous PING has not sparked any answer after a while, abort the client.
 */
static void test_ping_pong(time_t last_server_answer_time);

/* Update "last_server_answer_time" if new stuff has been written to the
 * server's out file.
 */
static void test_server_activity(time_t * last_server_answer_time);



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



static void read_log(char * read_buf, uint32_t linemax, FILE * file)
{
    free(world.log);
    world.log = NULL;
    while (try_fgets(read_buf, linemax + 1, file, __func__))
    {
        int old_size = 0;
        if (world.log)
        {
            old_size = strlen(world.log);
        }
        int new_size = strlen(read_buf);
        char * new_log = try_malloc(old_size + new_size + 1, __func__);
        memcpy(new_log, world.log, old_size);
        int test = sprintf(new_log + old_size, "%s", read_buf);
        exit_trouble(test < 0, __func__, "sprintf");
        free(world.log);
        world.log = new_log;
    }
}



static uint16_t read_value_from_line(char * read_buf, uint32_t linemax,
                                     FILE * file)
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



static uint8_t read_world()
{
    char * path = "server/worldstate";
    char * quit_msg = "No worldstate file found to read. Server may be down.";
    static uint8_t first_read = 1;
    exit_err(access(path, F_OK), quit_msg);
    FILE * file = changed_worldstate_file(path);
    if (!file)
    {
        return 0;
    }
    uint32_t linemax = textfile_width(file);
    char * read_buf = try_malloc(linemax + 1, __func__);
    world.turn = read_value_from_line(read_buf, linemax, file);
    world.player_lifepoints = read_value_from_line(read_buf, linemax, file);
    read_inventory(read_buf, linemax, file);
    world.player_pos.y = read_value_from_line(read_buf, linemax, file);
    world.player_pos.x = read_value_from_line(read_buf, linemax, file);
    if (1 == world.turn || first_read)
    {
        map_center();
        first_read = 0;
    }
    world.map.length = read_value_from_line(read_buf, linemax, file);
    read_map_cells(file, &world.map.cells);
    read_map_cells(file, &world.mem_map);
    read_log(read_buf, linemax, file);
    free(read_buf);
    try_fclose(file, __func__);
    return 1;
}



static void test_ping_pong(time_t last_server_answer_time)
{
    static uint8_t ping_sent = 0;
    time_t now = time(0);
    if (ping_sent && last_server_answer_time > now - 3)
    {
        ping_sent = 0;
    }
    if (!ping_sent && last_server_answer_time < now - 3)
    {
        send("PING");
        ping_sent = 1;
        return;
    }
    exit_err(last_server_answer_time < now - 6, "Server not answering.");
}



static void test_server_activity(time_t * last_server_answer_time)
{
    int test = try_fgetc(world.file_server_out, __func__);
    if (EOF == test)
    {
        return;
    }
    do
    {
        ;
    }
    while (EOF != (test = try_fgetc(world.file_server_out, __func__)));
    * last_server_answer_time = time(0);
}



extern void send(char * msg)
{
    uint32_t msg_size = strlen(msg) + 1;
    char * err = "send() tries to send message larger than PIPE_BUF bytes.";
    exit_err(msg_size > PIPE_BUF, err);
    try_fwrite(msg, strlen(msg), 1, world.file_server_in, __func__);
    try_fputc('\n', world.file_server_in, __func__);
    fflush(world.file_server_in);
}



extern char * io_loop()
{
    world.halfdelay = 1;            /* Ensures read_world() is only called 10 */
    halfdelay(world.halfdelay);     /* times a second during user inactivity. */
    uint8_t change_in_client = 0;
    uint16_t last_focused_turn = world.turn;
    time_t last_server_answer_time = time(0);
    while (1)
    {
        test_server_activity(&last_server_answer_time);
        test_ping_pong(last_server_answer_time);
        if (world.winch)
        {
            reset_windows_on_winch();
            world.winch = 0;
            change_in_client++;
        }
        if (change_in_client || read_world())
        {
            if (world.turn != last_focused_turn && world.focus_each_turn)
            {
                last_focused_turn = world.turn;
                map_center();
            }
            draw_all_wins();
        }
        change_in_client = 0;
        int key = getch();
        if (ERR != key)
        {
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
