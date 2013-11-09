/* misc.c */

#include "misc.h"
#include <errno.h> /* for errno */
#include <unistd.h> /* for unlink(), acess() */
#include <stdlib.h> /* for size_t, calloc(), free() */
#include <string.h> /* for strlen(), strcmp(), memcpy() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include "readwrite.h" /* for [read/write]_uint[8/16/32][_bigendian](),
                        * try_fopen(), try_fclose(), get_linemax()
                        */
#include "map_objects.h" /* for struct MapObj, get_player(), read_map_objects(),
                          * write_map_objects()
                          */
#include "map_object_actions.h" /* for is_passable(), move_actor() */
#include "map.h" /* for Map struct */
#include "main.h" /* for world global */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_err() */
#include "wincontrol.h" /* for init_winconfs(), init_wins(), free_winconfs(),
                         * sorted_wintoggle_and_activate()
                         */
#include "windows.h" /* for suspend_win() */
#include "command_db.h" /* for is_command_id_shortdsc() */



extern char * trouble_msg(char * parent, char * child)
{
    char * p1 = "Trouble in ";
    char * p2 = " with ";
    char * p3 = ".";
    uint16_t size = strlen(p1) + strlen(parent) + strlen(p2) + strlen(child)
                    + strlen(p3) + 1;
    char * msg = malloc(size);
    exit_err(NULL == msg, "malloc() in trouble_msg() failed.");
    sprintf(msg, "%s%s%s%s%s", p1, parent, p2, child, p3);
    return msg;
}



extern void * try_malloc(size_t size, char * f)
{
    char * msg = trouble_msg(f, "malloc()");
    void * p = malloc(size);
    exit_err(NULL == p, msg);
    free(msg);
    return p;
}



extern void * try_calloc(size_t size1, size_t size2, char * f)
{
    char * msg = trouble_msg(f, "calloc()");
    void * p = calloc(size1, size2);
    exit_err(NULL == p, msg);
    free(msg);
    return p;
}



extern void check_files_xor(char * p1, char * p2)
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
        exit_err(1, msg);
    }
    else if (access(p1, F_OK) && !access(p2, F_OK))
    {
        sprintf(msg, "%s%s%s%s%s", msg1, p2, msg2, p1, msg3);
        errno = 0;
        exit_err(1, msg);
    }
}



extern void check_tempfile(char * path)
{
    char * msg1 = "A file '";
    char * msg2 = "' exists, probably from a corrupted previous file saving "
                  "process. To avoid corruption of game files, the game won't "
                  "start until it is removed or renamed.";
    uint16_t size = strlen(msg1) + strlen(path) + strlen(msg2);
    char msg[size];
    sprintf(msg, "%s%s%s", msg1, path, msg2);
    exit_err(!access(path, F_OK), msg);
}



extern void save_interface_conf()
{
    save_keybindings("config/keybindings_global", &world.kb_global);
    save_keybindings("config/keybindings_wingeom", &world.kb_wingeom);
    save_keybindings("config/keybindings_winkeys", &world.kb_winkeys);
    save_win_configs();
}



extern void load_interface_conf()
{
    init_keybindings("config/keybindings_global",  &world.kb_global);
    init_keybindings("config/keybindings_wingeom", &world.kb_wingeom);
    init_keybindings("config/keybindings_winkeys", &world.kb_winkeys);
    init_winconfs();
    init_wins();
    sorted_wintoggle_and_activate();
}



extern void unload_interface_conf()
{
    free_keybindings(world.kb_global.kbs);
    free_keybindings(world.kb_wingeom.kbs);
    free_keybindings(world.kb_winkeys.kbs);
    while (0 != world.wmeta->active)
    {
        suspend_win(world.wmeta->active);
    }
    free_winconfs();
}



extern void reload_interface_conf()
{
    unload_interface_conf();
    load_interface_conf();
}



extern void update_log(char * text)
{
    char * f_name = "update_log()";
    static char * last_msg;                 /* TODO: valgrind is dissatisfied */
    if (0 == last_msg)                      /* with this calloc'd pointer not */
    {                                       /* being freed. Rectify this?     */
        last_msg = try_calloc(1, sizeof(char), f_name);
    }
    char * new_text;
    uint16_t len_old = strlen(world.log);
    if (0 == strcmp(last_msg, text))
    {
        uint16_t len_whole = len_old + 1;
        new_text = try_calloc(len_whole + 1, sizeof(char), f_name);
        memcpy(new_text, world.log, len_old);
        memcpy(new_text + len_old, ".", 1);
    }
    else
    {
        uint16_t len_new = strlen(text);
        uint16_t len_whole = len_old + len_new + 1;
        new_text = try_calloc(len_whole, sizeof(char), f_name);
        memcpy(new_text, world.log, len_old);
        memcpy(new_text + len_old, text, len_new);
        last_msg = try_calloc(len_new + 1, sizeof(char), f_name);
        memcpy(last_msg, text, len_new);
    }
    free(world.log);
    world.log = new_text;
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



extern void turn_over(char action)
{
    char * f_name = "turn_over()";
    char * err_write = "Trouble in turn_over() with write_uint8() "
                       "writing to opened file 'record_tmp'.";

    char * recordfile_tmp = "record_tmp";
    char * recordfile     = "record";
    if (1 == world.interactive)
    {
        FILE * file_old = try_fopen(recordfile,     "r", f_name);
        FILE * file_new = try_fopen(recordfile_tmp, "w", f_name);
        char c = fgetc(file_old);
        while (EOF != c)
        {
            exit_err(write_uint8(c, file_new), err_write);
            c = fgetc(file_old);
        }
        try_fclose(file_old, f_name);
        exit_err(write_uint8(action, file_new), err_write);
        if (is_command_id_shortdsc(action, "drop"))
        {
            uint8_t inventory_select = world.old_inventory_select;
            exit_err(write_uint8(inventory_select, file_new), err_write);
        }
        try_fclose_unlink_rename(file_new, recordfile_tmp, recordfile, f_name);
    }
    world.turn++;
    rrand_seed(world.seed * world.turn);
    struct MapObj * monster;
    for (monster = world.map_objs; monster != 0; monster = monster->next)
    {
        if (0 < monster->lifepoints && 0 != monster->id)
        {
            char * sel = "\0NSEW";
            move_actor(monster, sel[rrand() % 5]);
        }
    }
}



extern void save_game()
{
    char * f_name = "save_game()";
    char * savefile_tmp = "savefile_tmp";
    char * savefile     = "savefile";
    FILE * file = try_fopen(savefile_tmp, "w", f_name);
    char line[12];
    sprintf(line, "%d\n", world.seed);
    try_fwrite(line, strlen(line), 1, file, f_name);
    sprintf(line, "%d\n", world.turn);
    try_fwrite(line, strlen(line), 1, file, f_name);
    sprintf(line, "%d\n", world.score);
    try_fwrite(line, strlen(line), 1, file, f_name);
    write_map_objects(file);
    try_fclose_unlink_rename(file, savefile_tmp, savefile, f_name);
}



extern void load_game()
{
    char * f_name = "load_game2()";
    char * filename = "savefile";
    FILE * file = try_fopen(filename, "r", f_name);
    uint16_t linemax = get_linemax(file, f_name);
    char line[linemax + 1];
    try_fgets(line, linemax + 1, file, f_name);
    world.seed = atoi(line);
    try_fgets(line, linemax + 1, file, f_name);
    world.turn = atoi(line);
    try_fgets(line, linemax + 1, file, f_name);
    world.score = atoi(line);
    read_map_objects(file, line, linemax);
    try_fclose(file, f_name);
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



extern void nav_inventory(char dir)
{
    if ('u' == dir)
    {
        if (world.inventory_select > 0)
        {
            world.inventory_select--;
        }
        return;
    }
    struct MapObj * player = get_player();
    struct MapObj * owned = player->owns;
    if (NULL == owned)
    {
        return;
    }
    uint8_t n_owned = 0;
    for (; NULL != owned->next; owned = owned->next, n_owned++);
    if (world.inventory_select < n_owned)
    {
        world.inventory_select++;
    }
}
