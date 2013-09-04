/* misc.c */

#include "misc.h"
#include <stdio.h> /* for rename() */
#include <unistd.h> /* for unlink(), acess() */
#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strcmp(), memcpy() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include "readwrite.h" /* for [read/write]_uint[8/16/32][_bigendian]() */
#include "map_objects.h" /* for struct Monster, write_map_objects(), */
#include "map_object_actions.h" /* for is_passable(), move_monster() */
#include "map.h" /* for Map struct */
#include "main.h" /* for World struct */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_err() */



extern void update_log(struct World * world, char * text)
{
    static char * last_msg;                 /* TODO: valgrind is dissatisfied */
    if (0 == last_msg)                      /* with this calloc'd pointer     */
    {                                       /* never being freed.             */
        last_msg = calloc(1, sizeof(char)); /* Rectify this ?                 */
    }
    char * new_text;
    uint16_t len_old = strlen(world->log);
    if (0 == strcmp(last_msg, text))
    {
        uint16_t len_whole = len_old + 1;
        new_text = calloc(len_whole + 1, sizeof(char));
        memcpy(new_text, world->log, len_old);
        memcpy(new_text + len_old, ".", 1);
    }
    else
    {
        uint16_t len_new = strlen(text);
        uint16_t len_whole = len_old + len_new + 1;
        new_text = calloc(len_whole, sizeof(char));
        memcpy(new_text, world->log, len_old);
        memcpy(new_text + len_old, text, len_new);
        last_msg = calloc(len_new + 1, sizeof(char));
        memcpy(last_msg, text, len_new);
    }
    free(world->log);
    world->log = new_text;
}



extern uint16_t center_offset(uint16_t pos, uint16_t mapsize,
                              uint16_t framesize)
{
    uint16_t offset = 0;
    if (mapsize > framesize)
    {
        if (pos > framesize / 2)
        {
            if (pos < mapsize - (framesize / 2))
            {
                offset = pos - (framesize / 2);
            }
            else
            {
                offset = mapsize - framesize;
            }
        }
    }
    return offset;
}



extern void turn_over(struct World * world, char action)
{
    char * err_open  = "Trouble in turn_over() with fopen() "
                       "opening file 'record_tmp' for appending.";
    char * err_write = "Trouble in turn_over() with write_uint8() "
                       "writing to opened file 'record_tmp'.";
    char * err_close = "Trouble in turn_over() with fclose() "
                       "closing opened file 'record'.";
    char * err_unl   = "Trouble in turn_over() with unlink() "
                       "unlinking old file 'record'.";
    char * err_move  = "Trouble in turn_over() with rename() "
                       "renaming file 'record_tmp' to 'record'.";
    char * recordfile_tmp = "record_tmp";
    char * recordfile     = "record";
    if (1 == world->interactive)
    {
        FILE * file_old = fopen(recordfile, "r");
        FILE * file_new = fopen(recordfile_tmp, "w");
        exit_err(0 == file_old, world, err_open);
        char c = fgetc(file_old);
        while (EOF != c)
        {
            exit_err(write_uint8(c, file_new), world, err_write);
            c = fgetc(file_old);
        }
        exit_err(fclose(file_old), world, err_close);
        exit_err(write_uint8(action, file_new), world, err_write);
        err_close = "Trouble in turn_over() with fclose() "
                    "closing opened file 'record_tmp'.";
        exit_err(fclose(file_new), world, err_close);
        exit_err(unlink(recordfile), world, err_unl);
        exit_err(rename(recordfile_tmp, recordfile), world, err_move);
    }
    world->turn++;
    rrand_seed(world->seed * world->turn);
    struct Monster * monster;
    for (monster = world->monster;
         monster != 0;
         monster = monster->map_obj.next)
    {
        move_monster(world, monster);
    }
}



extern void save_game(struct World * world)
{
    char * err_open  = "Trouble in save_game() with fopen() "
                       "opening file 'savefile_tmp' for writing.";
    char * err_write = "Trouble in save_game() "
                       "writing to opened file 'savefile_tmp'.";
    char * err_close = "Trouble in save_game() with fclose() "
                       "closing opened file 'savefile_tmp'.";
    char * err_unl   = "Trouble in save_game() with unlink() "
                       "unlinking old 'savefile' file.";
    char * err_move  = "Trouble in save_game() with rename() "
                       "renaming 'file savefile_tmp' to 'savefile'.";
    char * savefile_tmp = "savefile_tmp";
    char * savefile     = "savefile";
    FILE * file = fopen(savefile_tmp, "w");
    exit_err(0 == file, world, err_open);
    if (   write_uint32_bigendian(world->seed, file)
        || write_uint32_bigendian(world->turn, file)
        || write_uint16_bigendian(world->score, file)
        || write_uint16_bigendian(world->player->pos.y + 1, file)
        || write_uint16_bigendian(world->player->pos.x + 1, file)
        || write_uint8(world->player->hitpoints, file)
        || write_map_objects(world, world->monster, file)
        || write_map_objects(world, world->item, file))
    {
        exit_err(1, world, err_write);
    }
    exit_err(fclose(file), world, err_close);
    if (!access(savefile, F_OK))
    {
        exit_err(unlink(savefile), world, err_unl);
    }
    exit_err(rename(savefile_tmp, savefile), world, err_move);
}



extern struct yx_uint16 find_passable_pos(struct Map * map)
{
    struct yx_uint16 pos;
    for (pos.y = pos.x = 0; 0 == is_passable(map, pos);)
    {
        pos.y = rrand() % map->size.y;
        pos.x = rrand() % map->size.x;
    }
    return pos;
}
