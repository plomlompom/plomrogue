/* main.c */

#include "main.h"
#include <stdlib.h> /* for atoi(), exit(), EXIT_FAILURE, calloc() */
#include <stdio.h> /* for FILE typedef, F_OK, rename() */
#include <ncurses.h> /* for initscr(), noecho(), curs_set(), keypad(), raw() */
#include <time.h> /* for time() */
#include <unistd.h> /* for getopt(), optarg */
#include <stdint.h> /* for uint16_t, uint32_t */
#include <errno.h> /* for errno */
#include "windows.h" /* for structs WinMeta, Win, init_win(), init_win_meta(),
                      * draw_all_wins()
                      */
#include "draw_wins.h" /* for draw_keys_win(), draw_map_win(), draw_info_win(),
                        * draw_log_win()
                        */
#include "keybindings.h" /* for init_keybindings(), get_action_key() */
#include "readwrite.h" /* for [read/write]_uint[8/16/32][_bigendian]() */
#include "map_objects.h" /* for structs Monster, Item, Player,
                          * init_map_object_defs(), read_map_objects(),
                          * build_map_objects()
                          */
#include "map.h" /* for struct Map, init_map() */
#include "misc.h" /* for update_log(), toggle_window(), find_passable_pos(),
                   * save_game()
                   */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_game() */
#include "control.h" /* for meta_control() */



int main(int argc, char *argv[])
{
    struct World world;

    /* Check for corrupted savefile / recordfile savings. */
    char * recordfile = "record";
    char * savefile = "savefile";
    char * recordfile_tmp = "record_tmp";
    char * savefile_tmp   = "savefile_tmp";
    char * err_x = "A file 'record' exists, but no 'savefile'. If everything "
                   "was in order, both or none would exist. I won't start "
                   "until this is corrected.";
    if (!access(recordfile, F_OK) && access(savefile, F_OK))
    {
        errno = 0;
        exit_err(1, &world, err_x);
    }
    err_x        = "A 'savefile' exists, but no file 'record'. If everything "
                   "was in order, both or none would exist. I won't start "
                   "until this is corrected.";
    if (!access(savefile, F_OK) && access(recordfile, F_OK))
    {
        errno = 0;
        exit_err(1, &world, err_x);
    }
    err_x        = "A file 'recordfile_tmp' exists, probably from a corrupted "
                   "previous record saving process. To avoid game record "
                   "corruption, I won't start until it is removed or renamed.";
    exit_err(!access(recordfile_tmp, F_OK), &world, err_x);
    err_x        = "A file 'savefile_tmp' exists, probably from a corrupted "
                   "previous game saving process. To avoid savegame "
                   "corruption, I won't start until it is removed or renamed.";
    exit_err(!access(savefile_tmp,   F_OK), &world, err_x);

    /* Read in startup options (i.e. replay option and replay start turn). */
    int opt;
    uint32_t start_turn;
    world.interactive = 1;
    while ((opt = getopt(argc, argv, "s::")) != -1)
    {
        switch (opt)
        {
            case 's':
            {
                world.interactive = 0;
                start_turn = 0;
                if (optarg)
                {
                    start_turn = atoi(optarg);
                }
                break;
            }
            default:
            {
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Initialize log, player, monster/item definitions and monsters/items. */
    world.score = 0;
    world.log = calloc(1, sizeof(char));
    set_cleanup_flag(CLEANUP_LOG);
    update_log (&world, " ");
    struct Player player;
    player.hitpoints = 5;
    world.player = &player;
    world.monster = 0;
    world.item = 0;
    init_map_object_defs(&world, "defs");

    /* For interactive mode, try to load world state from savefile. */
    char * err_o = "Trouble loading game (fopen() in main()) / "
                   "opening 'savefile' for reading.";
    char * err_r = "Trouble loading game (in main()) / "
                   "reading from opened 'savefile'.";
    char * err_c = "Trouble loading game (fclose() in main()) / "
                   "closing opened 'savefile'.";
    FILE * file;
    if (1 == world.interactive && 0 == access(savefile, F_OK))
    {
        file = fopen(savefile, "r");
        exit_err(0 == file, &world, err_o);
        if (   read_uint32_bigendian(file, &world.seed)
            || read_uint32_bigendian(file, &world.turn)
            || read_uint16_bigendian(file, &world.score)
            || read_uint16_bigendian(file, &player.pos.y)
            || read_uint16_bigendian(file, &player.pos.x)
            || read_uint8(file, &player.hitpoints)
            || read_map_objects(&world, &world.monster, file)
            || read_map_objects(&world, &world.item,    file))
        {
            exit_err(1, &world, err_r);
        }
        exit_err(fclose(file), &world, err_c);
        player.pos.y--;
        player.pos.x--;
    }

    /* For non-interactive mode, try to load world state from record file. */
    else
    {
        err_o = "Trouble loading record file (fopen() in main()) / "
                "opening file 'record' for reading.";
        err_r = "Trouble loading record file (read_uint32_bigendian() in "
                "main()) / reading from opened file 'record'.";
        world.turn = 1;
        if (0 == world.interactive)
        {
            file = fopen(recordfile, "r");
            exit_err(NULL == file, &world, err_o);
            exit_err(read_uint32_bigendian(file, &world.seed), &world, err_r);
        }

        /* For interactive-mode in newly started world, generate a start seed
         * from the current time.
         */
        else
        {
            world.seed = time(NULL);

            err_o        = "Trouble recording new seed (fopen() in main()) / "
                           "opening 'record_tmp' file for writing.";
            char * err_w = "Trouble recording new seed "
                           "(write_uint32_bigendian() in main()) / writing to "
                           "opened file 'record_tmp'.";
            err_c        = "Trouble recording new seed (fclose() in main()) / "
                           "closing opened file 'record_tmp'.";
            char * err_m = "Trouble recording new seed (rename() in main()) : "
                           "renaming file 'record_tmp' to 'record'.";
            file = fopen(recordfile_tmp, "w");
            exit_err(0 == file, &world, err_o);
            exit_err(write_uint32_bigendian(world.seed, file), &world, err_w);
            exit_err(fclose(file), &world, err_c);
            exit_err(rename(recordfile_tmp, recordfile), &world, err_m);
        }
    }


    /* Generate map from seed and, if newly generated world, start positions of
     * actors.
     */
    rrand_seed(world.seed);
    struct Map map = init_map();
    world.map = &map;
    set_cleanup_flag(CLEANUP_MAP);
    if (1 == world.turn)
    {
        player.pos = find_passable_pos(world.map);
        void * foo;
        foo = build_map_objects(&world, &world.monster, 1, 1 + rrand() % 27);
        foo = build_map_objects(&world, foo, 2, 1 + rrand() % 9);
        build_map_objects(&world, foo, 3, 1 + rrand() % 3);
        foo = build_map_objects(&world, &world.item, 4, 1 + rrand() % 3);
        build_map_objects(&world, foo, 5, 1 + rrand() % 3);
    }

    /* Initialize window system and windows. */
    WINDOW * screen = initscr();
    set_cleanup_flag(CLEANUP_NCURSES);
    noecho();
    curs_set(0);
    keypad(screen, TRUE);
    raw();
    init_keybindings(&world);
    set_cleanup_flag(CLEANUP_KEYBINDINGS);
    struct WinMeta win_meta;
    char * err_winmem = "Trouble with init_win:meta() or draw_all_wins() in "
                        "main().";
    exit_err(init_win_meta(screen, &win_meta), &world, err_winmem);
    world.wins.meta = &win_meta;
    struct Win win_keys = init_win(&win_meta, "Keys",
                                     0, 29, &world, draw_keys_win);
    world.wins.keys = &win_keys;
    struct Win win_info = init_win(&win_meta, "Info",
                                   3, 20, &world, draw_info_win);
    world.wins.info = &win_info;
    uint16_t height_logwin = win_meta.padframe.size.y
                             - (2 + win_info.frame.size.y);
    struct Win win_log = init_win(&win_meta, "Log",
                                  height_logwin, 20, &world, draw_log_win);
    world.wins.log = &win_log;
    uint16_t width_mapwin = win_meta.padframe.size.x - win_keys.frame.size.x
                            - win_log.frame.size.x - 2;
    struct Win win_map = init_win(&win_meta, "Map",
                                  0, width_mapwin, &world, draw_map_win);
    world.wins.map = &win_map;
    toggle_window(&win_meta, world.wins.keys);
    toggle_window(&win_meta, world.wins.map);
    toggle_window(&win_meta, world.wins.info);
    toggle_window(&win_meta, world.wins.log);

    /* Replay mode. */
    int key;
    if (0 == world.interactive)
    {
        int action = 0;
        if (0 != start_turn)
        {
            while (world.turn != start_turn)
            {
                action = getc(file);
                if (EOF == action)
                {
                    break;
                }
                record_control(action, &world);
            }
        }
        while (1)
        {
            draw_all_wins(&win_meta);
            key = getch();
            if (   EOF != action
                && key == get_action_key(world.keybindings, "wait / next turn"))
            {
                action = getc(file);
                if (EOF != action)
                {
                    record_control(action, &world);
                }
            }
            else if (meta_control(key, &world))
            {
                err_c = "Trouble closing 'record' file (fclose() in main()).";
                exit_err(fclose(file), &world, err_c);
                exit_game(&world);
            }
        }
    }

    /* Interactive mode. */
    else
    {
        while (1)
        {
            save_game(&world);
            draw_all_wins(&win_meta);
            key = getch();
            if (0 != player.hitpoints && 0 == player_control(key, &world))
            {
                continue;
            }
            if (meta_control(key, &world))
            {
                exit_game(&world);
            }
        }
    }
}
