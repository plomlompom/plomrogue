/* main.c */

#include "main.h" /* for world global */
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
                   * load_interface_conf(), load_game(), rrand()
                   */
#include "wincontrol.h" /* get_win_by_id(), get_winconf_by_win() */
#include "rexit.h" /* for exit_game(), exit_err() */
#include "command_db.h" /* for init_command_db(), is_command_id_shortdsc() */
#include "control.h" /* for control_by_id(), player_control(),
                      * get_available_keycode_to_action()
                      */
#include "map_object_actions.h" /* for init_map_object_actions() */



int main(int argc, char *argv[])
{
    char * f_name = "main()";
    world.turn = 0;        /* Turns to 1 when map and objects are initalized. */

    /* Initialize commands and map object actions. */
    init_command_db();
    set_cleanup_flag(CLEANUP_COMMAND_DB);
    init_map_object_actions();
    set_cleanup_flag(CLEANUP_MAPOBJACTS);

    /* Check for corrupted savefile / recordfile savings. */
    char * recordfile = "record";
    char * savefile = "savefile";
    char * recordfile_tmp = "record_tmp";
    char * savefile_tmp   = "savefile_tmp";
    check_files_xor(savefile, recordfile);
    check_tempfile(recordfile_tmp);
    check_tempfile(savefile_tmp);
    check_tempfile("config/windows/Win_tmp_k");
    check_tempfile("config/windows/Win_tmp_m");
    check_tempfile("config/windows/Win_tmp_i");
    check_tempfile("config/windows/Win_tmp_l");
    check_tempfile("config/windows/toggle_order_tmp");

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
    world.log = try_calloc(1, sizeof(char), f_name);
    set_cleanup_flag(CLEANUP_LOG);
    update_log(" ");
    init_map_object_defs("config/defs");
    set_cleanup_flag(CLEANUP_MAP_OBJECT_DEFS);
    world.map_obj_count = 0;

    /* For interactive mode, try to load world state from savefile. */
    char * err_r = "Trouble loading game (in main()) / "
                   "reading from opened 'savefile'.";
    FILE * file;
    if (1 == world.interactive && 0 == access(savefile, F_OK))
    {
        load_game();
        set_cleanup_flag(CLEANUP_MAP_OBJECTS);
    }

    /* For non-interactive mode, try to load world state from record file. */
    else
    {
        err_r = "Trouble reading from 'record' file (read_uint32_bigendian() "
                "in main()).";
        if (0 == world.interactive)
        {
            file = try_fopen(recordfile, "r", f_name);
            exit_err(read_uint32_bigendian(file, &world.seed), err_r);
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
            file = try_fopen(recordfile_tmp, "w", f_name);
            exit_err(write_uint32_bigendian(world.seed, file), err_w);
            try_fclose_unlink_rename(file, recordfile_tmp, recordfile, f_name);
        }
        world.mapseed = world.seed;
    }

    /* Generate map from seed and, if newly generated world, start positions of
     * actors.
     */
    uint32_t restore_seed = world.seed;
    world.seed = world.mapseed;
    struct Map map = init_map();
    world.map = &map;
    set_cleanup_flag(CLEANUP_MAP);
    if (0 == world.turn)
    {
        world.map_objs = NULL;
        add_map_objects(0, 1);
        add_map_objects(1, 1 + rrand() % 27);
        add_map_objects(2, 1 + rrand() % 9);
        add_map_objects(3, 1 + rrand() % 3);
        add_map_objects(4, 1 + rrand() % 3);
        add_map_objects(5, 1 + rrand() % 3);
        set_cleanup_flag(CLEANUP_MAP_OBJECTS);
        world.turn = 1;
    }
    world.seed = restore_seed;

    /* Initialize window system and windows. */
    init_win_meta();
    set_cleanup_flag(CLEANUP_NCURSES);
    noecho();
    curs_set(0);
    keypad(world.wmeta->screen, TRUE);
    raw();
    load_interface_conf();
    set_cleanup_flag(CLEANUP_INTERFACE);

    /* Focus map on player. */
    struct MapObj * player = get_player();
    struct Win * win_map = get_win_by_id('m');
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
                if (   is_command_id_shortdsc(action, "drop")
                    || is_command_id_shortdsc(action, "use"))
                {
                    world.inventory_select = getc(file);
                }
                player_control_by_id(action);
            }
        }
        while (1)
        {
            draw_all_wins();
            key = getch();
            wc = get_winconf_by_win(world.wmeta->active);
            if  (   (1 == wc->view && wingeom_control(key))
                 || (2 == wc->view && winkeyb_control(key)))
            {
                continue;
            }
            if (   EOF != action
                && key == get_available_keycode_to_action("wait"))
            {
                action = getc(file);
                if (EOF != action)
                {
                    if (   is_command_id_shortdsc(action, "drop")
                        || is_command_id_shortdsc(action, "use"))
                    {
                        world.inventory_select = getc(file);
                    }
                    player_control_by_id(action);
                }
            }
            else if (meta_control(key))
            {
                try_fclose(file, f_name);
                exit_game();
            }
        }
    }

    /* Interactive mode. */
    else
    {
        while (1)
        {
            save_game();
            draw_all_wins();
            key = getch();
            wc = get_winconf_by_win(world.wmeta->active);
            if  (   (1 == wc->view && wingeom_control(key))
                 || (2 == wc->view && winkeyb_control(key))
                 || (0 != player->lifepoints && player_control_by_key(key)))
            {
                continue;
            }
            if (meta_control(key))
            {
                exit_game();
            }
        }
    }
}
