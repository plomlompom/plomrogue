/* misc.c */

#include "misc.h"
#include <errno.h> /* for errno */
#include <unistd.h> /* for unlink(), acess() */
#include <stdlib.h> /* for size_t, calloc(), free() */
#include <string.h> /* for strlen(), strcmp(), memcpy() */
#include <stdint.h> /* for uint8_t, uint16_t */
#include "readwrite.h" /* for try_fopen(), try_fclose(), textfile_sizes(),
                        * try_fputc(), try_fgetc()
                        */
#include "map_objects.h" /* for struct MapObj, get_player(), read_map_objects(),
                          * write_map_objects()
                          */
#include "map_object_actions.h" /* for struct MapObjAct */
#include "ai.h" /* for pretty_dumb_ai() */
#include "map.h" /* for Map struct, is_passable() */
#include "main.h" /* for world global */
#include "yx_uint16.h" /* for yx_uint16 struct */
#include "rexit.h" /* for exit_err(), exit_trouble() */
#include "wincontrol.h" /* for init_winconfs(), init_wins(), free_winconfs(),
                         * sorted_wintoggle_and_activate()
                         */
#include "windows.h" /* for suspend_win() */
#include "command_db.h" /* for is_command_id_shortdsc() */



extern uint16_t rrand()
{   /* Constants as recommended by POSIX.1-2001 (see man page rand(3)). */
    world.seed = ((world.seed * 1103515245) + 12345) % 4294967296;
    return (world.seed >> 16); /* Ignore less random least significant bits. */
}



extern void * try_malloc(size_t size, char * f)
{
    void * p = malloc(size);
    exit_trouble(NULL == p, f, "malloc()");
    return p;
}



extern void * try_calloc(size_t size1, size_t size2, char * f)
{
    void * p = calloc(size1, size2);
    exit_trouble(NULL == p, f, "calloc()");
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



extern uint16_t center_offset(uint16_t position, uint16_t mapsize,
                              uint16_t framesize)
{
    uint16_t offset = 0;
    if (mapsize > framesize)
    {
        if (position > framesize / 2)
        {
            if (position < mapsize - (framesize / 2))
            {
                offset = position - (framesize / 2);
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

    char * recordfile_tmp = "record_tmp";
    char * recordfile     = "record";
    if (1 == world.interactive)
    {
        FILE * file_old = try_fopen(recordfile,     "r", f_name);
        FILE * file_new = try_fopen(recordfile_tmp, "w", f_name);
        int c = try_fgetc(file_old, f_name);
        while (EOF != c)
        {
            try_fputc((uint8_t) c, file_new, f_name);
            c = try_fgetc(file_old, f_name);
        }
        try_fclose(file_old, f_name);
        try_fputc(action, file_new, f_name);
        if (   is_command_id_shortdsc(action, "drop")
            || is_command_id_shortdsc(action, "use"))
        {
            try_fputc(world.inventory_sel, file_new, f_name);
        }
        try_fclose_unlink_rename(file_new, recordfile_tmp, recordfile, f_name);
    }

    struct MapObj * player = get_player();
    struct MapObj * map_object = player;
    uint8_t first_round = 1;
    while (0 < player->lifepoints)
    {
        if (NULL == map_object)
        {
            world.turn++;
            map_object = world.map_objs;
        }
        if (0 < map_object->lifepoints)             /* map_object is animate. */
        {
            if (0 == first_round && 0 == map_object->progress)
            {
                if (map_object == player)
                {
                    break;
                }
                pretty_dumb_ai(map_object);
            }
            first_round = 0;
            map_object->progress++;
            struct MapObjAct * moa = world.map_obj_acts;
            while (moa->id != map_object->command)
            {
                moa = moa->next;
            }
            if (map_object->progress == moa->effort)
            {
                moa->func(map_object);
                map_object->progress = 0;
            }
        }
        map_object = map_object->next;
    }
}



extern void save_game()
{
    char * f_name = "save_game()";
    char * savefile_tmp = "savefile_tmp";
    char * savefile     = "savefile";
    FILE * file = try_fopen(savefile_tmp, "w", f_name);
    char line[12];
    sprintf(line, "%u\n", world.mapseed);
    try_fwrite(line, strlen(line), 1, file, f_name);
    sprintf(line, "%u\n", world.seed);
    try_fwrite(line, strlen(line), 1, file, f_name);
    sprintf(line, "%u\n", world.turn);
    try_fwrite(line, strlen(line), 1, file, f_name);
    sprintf(line, "%u\n", world.score);
    try_fwrite(line, strlen(line), 1, file, f_name);
    write_map_objects(file);
    try_fclose_unlink_rename(file, savefile_tmp, savefile, f_name);
}



extern void load_game()
{
    char * f_name = "load_game2()";
    char * filename = "savefile";
    FILE * file = try_fopen(filename, "r", f_name);
    uint16_t linemax = textfile_sizes(file, NULL);
    char line[linemax + 1];
    try_fgets(line, linemax + 1, file, f_name);
    world.mapseed = atoi(line);
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
        world.inventory_sel = world.inventory_sel - (world.inventory_sel > 0);
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
    world.inventory_sel = world.inventory_sel + (world.inventory_sel < n_owned);
}
