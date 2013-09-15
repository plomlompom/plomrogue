/* main.c */

#include "main.h"
#include <stdlib.h> /* for atoi(), exit(), EXIT_FAILURE */
#include <stdio.h> /* for FILE typedef, F_OK */
#include <ncurses.h> /* for initscr(), noecho(), curs_set(), keypad(), raw() */
#include <time.h> /* for time() */
#include <unistd.h> /* for getopt(), optarg */
#include <stdint.h> /* for uint16_t, uint32_t */
#include "windows.h" /* for structs WinMeta, Win, init_win(), init_win_meta(),
                      * draw_all_wins()
                      */
#include "readwrite.h" /* for [read/write]_uint[8/16/32][_bigendian](),
                        * try_fopen(), try_fclose(), try_fclose_unlink_rename()
                        */
#include "map_objects.h" /* for structs Monster, Item, Player,
                          * init_map_object_defs(), read_map_objects(),
                          * build_map_objects()
                          */
#include "map.h" /* for struct Map, init_map() */
#include "misc.h" /* for update_log(), find_passable_pos(), save_game(),
                   * try_calloc(), check_tempfile(), check_xor_files()
                   */
#include "wincontrol.h" /* for create_winconfs(), init_winconfs(), init_wins(),
                         * sorted_wintoggle()
                         */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_game(), exit_err() */
#include "command_db.h" /* for init_command_db() */
#include "control.h" /* for *_control() */
#include "keybindings.h" /* for init_keybindings(),
                          * get_available_keycode_to_action()
                          */



int main(int argc, char *argv[])
{
    char * f_name = "main()";
    struct World world;
    world.turn = 0;        /* Turns to 1 when map and objects are initalized. */

    init_command_db(&world);
    set_cleanup_flag(CLEANUP_COMMAND_DB);

    /* Check for corrupted savefile / recordfile savings. */
    char * recordfile = "record";
    char * savefile = "savefile";
    char * recordfile_tmp = "record_tmp";
    char * savefile_tmp   = "savefile_tmp";
    check_files_xor(savefile, recordfile, &world);
    check_tempfile(recordfile_tmp, &world);
    check_tempfile(savefile_tmp, &world);
    check_tempfile("config/windows/Win_tmp_k", &world);
    check_tempfile("config/windows/Win_tmp_m", &world);
    check_tempfile("config/windows/Win_tmp_i", &world);
    check_tempfile("config/windows/Win_tmp_l", &world);
    check_tempfile("config/windows/toggle_order_tmp", &world);

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
    world.log = try_calloc(1, sizeof(char), &world, f_name);
    set_cleanup_flag(CLEANUP_LOG);
    update_log(&world, " ");
    struct Player player;
    player.hitpoints = 5;
    world.player = &player;
    world.monster = 0;
    world.item = 0;
    init_map_object_defs(&world, "config/defs");
    set_cleanup_flag(CLEANUP_MAP_OBJECT_DEFS);

    /* For interactive mode, try to load world state from savefile. */
    char * err_r = "Trouble loading game (in main()) / "
                   "reading from opened 'savefile'.";
    FILE * file;
    if (1 == world.interactive && 0 == access(savefile, F_OK))
    {
        file = try_fopen(savefile, "r", &world, f_name);
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
        set_cleanup_flag(CLEANUP_MAP_OBJECTS);
        try_fclose(file, &world, f_name);
        player.pos.y--;
        player.pos.x--;
    }

    /* For non-interactive mode, try to load world state from record file. */
    else
    {
        err_r = "Trouble reading from 'record' file (read_uint32_bigendian() "
                "in main()).";
        if (0 == world.interactive)
        {
            file = try_fopen(recordfile, "r", &world, f_name);
            exit_err(read_uint32_bigendian(file, &world.seed), &world, err_r);
        }

        /* For interactive-mode in newly started world, generate a start seed
         * from the current time.
         */
        else
        {
            world.seed = time(NULL);

            char * err_w = "Trouble recording new seed "
                           "(write_uint32_bigendian() in main()) / writing to "
                           "file 'record_tmp'.";
            file = try_fopen(recordfile_tmp, "w", &world, f_name);
            exit_err(write_uint32_bigendian(world.seed, file), &world, err_w);
            try_fclose_unlink_rename(file, recordfile_tmp, recordfile,
                                    &world, f_name);
        }
    }

    /* Generate map from seed and, if newly generated world, start positions of
     * actors.
     */
    rrand_seed(world.seed);
    struct Map map = init_map();
    world.map = &map;
    set_cleanup_flag(CLEANUP_MAP);
    if (0 == world.turn)
    {
        player.pos = find_passable_pos(world.map);
        void * foo;
        foo = build_map_objects(&world, &world.monster, 1, 1 + rrand() % 27);
        foo = build_map_objects(&world, foo, 2, 1 + rrand() % 9);
        build_map_objects(&world, foo, 3, 1 + rrand() % 3);
        foo = build_map_objects(&world, &world.item, 4, 1 + rrand() % 3);
        build_map_objects(&world, foo, 5, 1 + rrand() % 3);
        set_cleanup_flag(CLEANUP_MAP_OBJECTS);
        world.turn = 1;
    }

    /* Initialize window system and windows. */
    WINDOW * screen = initscr();
    set_cleanup_flag(CLEANUP_NCURSES);
    noecho();
    curs_set(0);
    keypad(screen, TRUE);
    raw();
    init_keybindings(&world, "config/keybindings_global",  &world.kb_global);
    init_keybindings(&world, "config/keybindings_wingeom", &world.kb_wingeom);
    init_keybindings(&world, "config/keybindings_winkeys", &world.kb_winkeys);
    set_cleanup_flag(CLEANUP_KEYBINDINGS);
    char * err_winmem = "Trouble with init_win_meta() in main ().";
    exit_err(init_win_meta(screen, &world.wmeta), &world, err_winmem);
    set_cleanup_flag(CLEANUP_WIN_META);
    init_winconfs(&world);
    init_wins(&world);
    set_cleanup_flag(CLEANUP_WINCONFS);
    sorted_wintoggle(&world);
    err_winmem = "Trouble with draw_all_wins() in main().";

    /* Replay mode. */
    int key;
    struct WinConf * wc;
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
            draw_all_wins(world.wmeta);
            key = getch();
            wc = get_winconf_by_win(&world, world.wmeta->active);
            if  (   (1 == wc->view && wingeom_control(key, &world))
                 || (2 == wc->view && winkeyb_control(key, &world)))
            {
                continue;
            }
            if (   EOF != action
                && key == get_available_keycode_to_action(&world, "wait"))
            {
                action = getc(file);
                if (EOF != action)
                {
                    record_control(action, &world);
                }
            }
            else if (meta_control(key, &world))
            {
                try_fclose(file, &world, f_name);
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
            draw_all_wins(world.wmeta);
            key = getch();
            wc = get_winconf_by_win(&world, world.wmeta->active);
            if      (1 == wc->view && wingeom_control(key, &world))
            {
                continue;
            }
            else if (2 == wc->view && winkeyb_control(key, &world))
            {
                continue;
            }

            if  (   (1 == wc->view && wingeom_control(key, &world))
                 || (2 == wc->view && winkeyb_control(key, &world))
                 || (0 != player.hitpoints && player_control(key, &world)))
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
