/* main.c */

#include "main.h"
#include <stdlib.h> /* for atoi(), exit(), EXIT_FAILURE */
#include <stdio.h> /* for FILE typedef, F_OK */
#include <ncurses.h> /* for initscr(), noecho(), curs_set(), keypad(), raw() */
#include <time.h> /* for time() */
#include <unistd.h> /* for getopt(), optarg */
#include <stdint.h> /* for uint32_t */
#include "windows.h" /* for structs WinMeta, Win, init_win_meta(),
                      * draw_all_wins()
                      */
#include "readwrite.h" /* for read_uint32_bigendian](),
                        * write_uint32_bigendian(), try_fopen(), try_fclose(),
                        * try_fclose_unlink_rename()
                        */
#include "map_objects.h" /* for structs MapObj, init_map_object_defs(),
                          * build_map_objects(), get_player()
                          */
#include "map.h" /* for struct Map, init_map() */
#include "misc.h" /* for update_log(), find_passable_pos(), save_game(),
                   * try_calloc(), check_tempfile(), check_xor_files(),
                   * load_interface_conf(), load_game()
                   */
#include "wincontrol.h" /* get_win_by_id(), get_winconf_by_win() */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_game(), exit_err() */
#include "command_db.h" /* for init_command_db(), is_command_id_shortdsc() */
#include "control.h" /* for *_control(), get_available_keycode_to_action() */



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

    /* Initialize log and map object definitions. */
    world.score = 0;
    world.log = try_calloc(1, sizeof(char), &world, f_name);
    set_cleanup_flag(CLEANUP_LOG);
    update_log(&world, " ");
    init_map_object_defs(&world, "config/defs");
    set_cleanup_flag(CLEANUP_MAP_OBJECT_DEFS);
    world.map_obj_count = 0;

    /* For interactive mode, try to load world state from savefile. */
    char * err_r = "Trouble loading game (in main()) / "
                   "reading from opened 'savefile'.";
    FILE * file;
    if (1 == world.interactive && 0 == access(savefile, F_OK))
    {
        load_game(&world);
        set_cleanup_flag(CLEANUP_MAP_OBJECTS);
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
        world.map_objs = NULL;
        add_map_objects(&world, 0, 1);
        add_map_objects(&world, 1, 1 + rrand() % 27);
        add_map_objects(&world, 2, 1 + rrand() % 9);
        add_map_objects(&world, 3, 1 + rrand() % 3);
        add_map_objects(&world, 4, 1 + rrand() % 3);
        add_map_objects(&world, 5, 1 + rrand() % 3);
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
    char * err_winmem = "Trouble with init_win_meta() in main ().";
    exit_err(init_win_meta(screen, &world.wmeta), &world, err_winmem);
    set_cleanup_flag(CLEANUP_WIN_META);
    load_interface_conf(&world);
    set_cleanup_flag(CLEANUP_INTERFACE_CONF);
    err_winmem = "Trouble with draw_all_wins() in main().";

    /* Focus map on player. */
    struct MapObj * player = get_player(&world);
    struct Win * win_map = get_win_by_id(&world, 'm');
    win_map->center = player->pos;

    /* Initialize player's inventory selection index to start position. */
    world.inventory_select = 0;

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
                if (is_command_id_shortdsc(&world, action, "drop"))
                {
                    world.inventory_select = getc(file);
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
                    if (is_command_id_shortdsc(&world, action, "drop"))
                    {
                        world.inventory_select = getc(file);
                    }
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
                 || (0 != player->lifepoints && player_control(key, &world)))
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
