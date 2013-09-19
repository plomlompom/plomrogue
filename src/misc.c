/* misc.c */

#include "misc.h"
#include <errno.h> /* for errno */
#include <unistd.h> /* for unlink(), acess() */
#include <stdlib.h> /* for size_t, calloc(), free() */
#include <string.h> /* for strlen(), strcmp(), memcpy() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include "readwrite.h" /* for [read/write]_uint[8/16/32][_bigendian](),
                        * try_fopen(), try_fclose()
                        */
#include "map_objects.h" /* for struct Monster, write_map_objects(), */
#include "map_object_actions.h" /* for is_passable(), move_monster() */
#include "map.h" /* for Map struct */
#include "main.h" /* for World struct */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_err() */
#include "wincontrol.h" /* for init_winconfs(), init_wins(), free_winconfs(),
                         * sorted_wintoggle_and_activate()
                         */
#include "windows.h" /* for suspend_win() */


extern char * trouble_msg(struct World * w, char * parent, char * child)
{
    char * p1 = "Trouble in ";
    char * p2 = " with ";
    char * p3 = ".";
    uint16_t size = strlen(p1) + strlen(parent) + strlen(p2) + strlen(child)
                    + strlen(p3) + 1;
    char * msg = malloc(size);
    exit_err(NULL == msg, w, "malloc() in trouble_msg() failed.");
    sprintf(msg, "%s%s%s%s%s", p1, parent, p2, child, p3);
    return msg;
}



extern void * try_malloc(size_t size, struct World * w, char * f)
{
    char * msg = trouble_msg(w, f, "malloc()");
    void * p = malloc(size);
    exit_err(NULL == p, w, msg);
    free(msg);
    return p;
}



extern void * try_calloc(size_t size1, size_t size2, struct World * w, char * f)
{
    char * msg = trouble_msg(w, f, "calloc()");
    void * p = calloc(size1, size2);
    exit_err(NULL == p, w, msg);
    free(msg);
    return p;
}



extern void check_files_xor(char * p1, char * p2, struct World * w)
{
    char * msg1 = "A file '";
    char * msg2 = "' exists, but no file '";
    char * msg3 = "'. If everything was in order, both or noe would exist. "
                  "The game won't start until this is corrected.";
    uint16_t size = strlen(msg1) + strlen(p1) + strlen(msg2) + strlen(p2)
                    + strlen(msg3);
    char msg[size];
    if      (!access(p1, F_OK) && access(p2, F_OK))
    {
        sprintf(msg, "%s%s%s%s%s", msg1, p1, msg2, p2, msg3);
        errno = 0;
        exit_err(1, w, msg);
    }
    else if (access(p1, F_OK) && !access(p2, F_OK))
    {
        sprintf(msg, "%s%s%s%s%s", msg1, p2, msg2, p1, msg3);
        errno = 0;
        exit_err(1, w, msg);
    }
}



extern void check_tempfile(char * path, struct World * w)
{
    char * msg1 = "A file '";
    char * msg2 = "' exists, probably from a corrupted previous file saving "
                "process. To avoid corruption of game files, the game won't  "
                "start until it is removed or renamed.";
    uint16_t size = strlen(msg1) + strlen(path) + strlen(msg2);
    char msg[size];
    sprintf(msg, "%s%s%s", msg1, path, msg2);
    exit_err(!access(path, F_OK), w, msg);
}



extern void save_interface_conf(struct World * world)
{
    save_keybindings(world, "config/keybindings_global", &world->kb_global);
    save_keybindings(world, "config/keybindings_wingeom", &world->kb_wingeom);
    save_keybindings(world, "config/keybindings_winkeys", &world->kb_winkeys);
    save_win_configs(world);
}



extern void load_interface_conf(struct World * world)
{
    init_keybindings(world, "config/keybindings_global",  &world->kb_global);
    init_keybindings(world, "config/keybindings_wingeom", &world->kb_wingeom);
    init_keybindings(world, "config/keybindings_winkeys", &world->kb_winkeys);
    init_winconfs(world);
    init_wins(world);
    sorted_wintoggle_and_activate(world);
}



extern void unload_interface_conf(struct World * world)
{
    free_keybindings(world->kb_global.kbs);
    free_keybindings(world->kb_wingeom.kbs);
    free_keybindings(world->kb_winkeys.kbs);
    while (0 != world->wmeta->active)
    {
        suspend_win(world->wmeta, world->wmeta->active);
    }
    free_winconfs(world);
}



extern void update_log(struct World * world, char * text)
{
    char * f_name = "update_log()";
    static char * last_msg;                 /* TODO: valgrind is dissatisfied */
    if (0 == last_msg)                      /* with this calloc'd pointer not */
    {                                       /* being freed. Rectify this?     */
        last_msg = try_calloc(1, sizeof(char), world, f_name);
    }
    char * new_text;
    uint16_t len_old = strlen(world->log);
    if (0 == strcmp(last_msg, text))
    {
        uint16_t len_whole = len_old + 1;
        new_text = try_calloc(len_whole + 1, sizeof(char), world, f_name);
        memcpy(new_text, world->log, len_old);
        memcpy(new_text + len_old, ".", 1);
    }
    else
    {
        uint16_t len_new = strlen(text);
        uint16_t len_whole = len_old + len_new + 1;
        new_text = try_calloc(len_whole, sizeof(char), world, f_name);
        memcpy(new_text, world->log, len_old);
        memcpy(new_text + len_old, text, len_new);
        last_msg = try_calloc(len_new + 1, sizeof(char), world, f_name);
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
    char * f_name = "turn_over()";
    char * err_write = "Trouble in turn_over() with write_uint8() "
                       "writing to opened file 'record_tmp'.";

    char * recordfile_tmp = "record_tmp";
    char * recordfile     = "record";
    if (1 == world->interactive)
    {
        FILE * file_old = try_fopen(recordfile,     "r", world, f_name);
        FILE * file_new = try_fopen(recordfile_tmp, "w", world, f_name);
        char c = fgetc(file_old);
        while (EOF != c)
        {
            exit_err(write_uint8(c, file_new), world, err_write);
            c = fgetc(file_old);
        }
        try_fclose(file_old, world, f_name);
        exit_err(write_uint8(action, file_new), world, err_write);
        try_fclose_unlink_rename(file_new, recordfile_tmp, recordfile,
                                 world, f_name);
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
    char * f_name = "save_game()";
    char * err_write = "Trouble in save_game() "
                       "writing to opened file 'savefile_tmp'.";

    char * savefile_tmp = "savefile_tmp";
    char * savefile     = "savefile";
    FILE * file = try_fopen(savefile_tmp, "w", world, f_name);
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
    try_fclose_unlink_rename(file, savefile_tmp, savefile, world, f_name);
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
