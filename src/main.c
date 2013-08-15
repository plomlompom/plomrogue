/* main.c */

#include "main.h"
#include <stdlib.h> /* for atoi(), exit(), EXIT_FAILURE, calloc() */
#include <stdio.h> /* for FILE typedef, F_OK */
#include <ncurses.h> /* for initscr(), noecho(), curs_set(), keypad(), raw() */
#include <time.h> /* for time() */
#include <unistd.h> /* for getopt(), optarg */
#include "windows.h" /* for structs WinMeta, Win, init_win(), init_win_meta(),
                      * draw_all_wins()
                      */
#include "draw_wins.h" /* for draw_keys_win(), draw_map_win(), draw_info_win(),
                        * draw_log_win()
                        */
#include "keybindings.h" /* for initkeybindings(), get_action_key() */
#include "readwrite.h" /* for [read/write]_uint[8/16/32][_bigendian]() */
#include "map_objects.h" /* for structs Monster, Item, Player,
                          * init_map_object_defs(), read_map_objects(),
                          * build_map_objects()
                          */
#include "map_object_actions.h" /* for player_wait(), move_player() */
#include "map.h" /* for struct Map, init_map() */
#include "misc.h" /* for update_log(), toggle_window(), find_passable_pos(),
                   * meta_keys(), save_game()
                   */
#include "yx_uint16.h" /* for dir enum */
#include "rrand.h" /* for rrand(), rrand_seed() */
#include "rexit.h" /* for exit_game() */

int main(int argc, char *argv[])
{
    struct World world;

    /* Read in startup options (i.e. replay option and replay start turn). */
    int opt;
    uint32_t start_turn;
    world.interactive = 1;
    while ((opt = getopt(argc, argv, "s::")) != -1)
    {
        switch (opt)
        {
            case 's':
                world.interactive = 0;
                start_turn = 0;
                if (optarg)
                    start_turn = atoi(optarg);
                 break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    /* Initialize log, player, monster/item definitions and monsters/items. */
    world.log = calloc(1, sizeof(char));
    set_cleanup_flag(CLEANUP_LOG);
    update_log (&world, " ");
    struct Player player;
    player.hitpoints = 5;
    world.player = &player;
    world.monster = 0;
    world.item = 0;
    init_map_object_defs(&world, "defs");
    uint8_t err = 0;

    /* For interactive mode, try to load world state from savefile. */
    FILE * file;
    if (1 == world.interactive && 0 == access("savefile", F_OK))
    {
        file = fopen("savefile", "r");
        err = err | read_uint32_bigendian(file, &world.seed);
        err = err | read_uint32_bigendian(file, &world.turn);
        err = err | read_uint16_bigendian(file, &player.pos.y);
        err = err | read_uint16_bigendian(file, &player.pos.x);
        player.pos.y--;
        player.pos.x--;
        err = err | read_uint8(file, &player.hitpoints);
        err = err | read_map_objects(&world, &world.monster, file);
        err = err | read_map_objects(&world, &world.item,    file);
        fclose(file);
    }

    /* For non-interactive mode, try to load world state from record file. */
    else
    {
        world.turn = 1;
        if (0 == world.interactive)
        {
            file = fopen("record", "r");
            err = err | read_uint32_bigendian(file, &world.seed);
        }

        /* For interactive-mode in newly started world, generate a start seed
         * from the current time.
         */
        else
        {
            file = fopen("record", "w");
            world.seed = time(NULL);
            err = err | write_uint32_bigendian(world.seed, file);
            fclose(file);
        }
    }

    exit_err(err, &world, "Failure initializing game.");


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
    struct WinMeta win_meta = init_win_meta(screen);
    struct Win win_keys = init_win(&win_meta, "Keys",
                                   0, 29, &world, draw_keys_win);
    struct Win win_info = init_win(&win_meta, "Info",
                                   2, 20, &world, draw_info_win);
    uint16_t height_logwin = win_meta.padframe.size.y
                             - (2 + win_info.frame.size.y);
    struct Win win_log = init_win(&win_meta, "Log",
                                  height_logwin, 20, &world, draw_log_win);
    uint16_t width_mapwin = win_meta.padframe.size.x - win_keys.frame.size.x
                             - win_log.frame.size.x - 2;
    struct Win win_map = init_win(&win_meta, "Map",
                                  0, width_mapwin, &world, draw_map_win);
    toggle_window(&win_meta, &win_keys);
    toggle_window(&win_meta, &win_map);
    toggle_window(&win_meta, &win_info);
    toggle_window(&win_meta, &win_log);

    /* Replay mode. */
    int key;
    unsigned char quit_called = 0;
    unsigned char await_actions = 1;
    if (0 == world.interactive)
    {
        int action;
        while (1)
        {
            if (start_turn == world.turn)
            {
                start_turn = 0;
            }
            if (0 == start_turn)
            {
                draw_all_wins (&win_meta);
               key = getch();
            }
            if (1 == await_actions
                && (world.turn < start_turn
                    || key == get_action_key(world.keybindings,
                                             "wait / next turn")) )
            {
                action = getc(file);
                if (EOF == action)
                {
                    start_turn = 0;
                    await_actions = 0;
                }
                else if (0 == action)
                {
                    player_wait (&world);
                }
                else if (NORTH == action)
                {
                    move_player(&world, NORTH);
                }
                else if (EAST  == action)
                {
                    move_player(&world, EAST);
                }
                else if (SOUTH == action)
                {
                    move_player(&world, SOUTH);
                }
                else if (WEST == action)
                {
                    move_player(&world, WEST);
                }
            }
            else
            {
                quit_called = meta_keys(key, &world, &win_meta, &win_keys,
                                             &win_map, &win_info, &win_log);
                if (1 == quit_called)
                {
                    exit_game(&world);
                }
            }
        }
    }

    /* Interactive mode. */
    else
    {
        uint32_t last_turn = 0;
        while (1)
        {
            if (last_turn != world.turn)
            {
                save_game(&world);
                last_turn = world.turn;
            }
            if (1 == await_actions && 0 == player.hitpoints)
            {
                await_actions = 0;
            }
            draw_all_wins (&win_meta);
            key = getch();
            if      (1 == await_actions
                     && key == get_action_key(world.keybindings,
                                              "player up"))
            {
                move_player(&world, NORTH);
            }
            else if (1 == await_actions
                     && key == get_action_key(world.keybindings,
                                              "player right"))
            {
                move_player(&world, EAST);
            }
            else if (1 == await_actions
                     && key == get_action_key(world.keybindings,
                                              "player down"))
            {
                move_player(&world, SOUTH);
            }
            else if (1 == await_actions
                     && key == get_action_key(world.keybindings,
                                              "player left"))
            {
                move_player(&world, WEST);
            }
            else if (1 == await_actions
                     && key == get_action_key(world.keybindings,
                                              "wait / next turn"))
            {
                player_wait (&world);
            }
            else
            {
                quit_called = meta_keys(key, &world, &win_meta, &win_keys,
                                             &win_map, &win_info, &win_log);
                if (1 == quit_called)
                {
                    exit_game(&world);
                }
            }
        }
    }
}
