/* main.c */

#define _POSIX_C_SOURCE 1 /* struct sigaction, sigaction() */
#include <ncurses.h> /* keypad() */
#include <signal.h> /* struct sigaction, sigaction() */
#include <stddef.h> /* NULL */
#include <stdlib.h> /* exit() */
#include <string.h> /* memset() */
#include <unistd.h> /* access() */
#include "../common/readwrite.h" /* try_fopen() */
#include "../common/rexit.h" /* set_cleanup_func(), exit_trouble(),exit_err() */
#include "cleanup.h" /* cleanup(), set_cleanup_flag() */
#include "command_db.h" /* init_command_db() */
#include "interface_conf.h" /* load_interface_conf(), obey_argv() */
#include "io.h" /* io_loop() */
#include "windows.h" /* winch_called() */
#include "world.h" /* struct World */



struct World world;



int main(int argc, char * argv[])
{
    char * f_name = "main()";

    /* Declare hard-coded paths and values here. */
    world.path_commands    = "confclient/commands";
    world.path_interface   = "confclient/interface_conf";
    world.winDB.legal_ids  = "012ciklm";
    char * path_server_in  = "server/in";
    char * path_server_out = "server/out";

    /* Parse command line arguments. */
    obey_argv(argc, argv);

    /* So error exits also go through the client's cleanup() function. */
    set_cleanup_func(cleanup);

    /* Check existence of config files. */
    exit_err(access(world.path_commands, F_OK), "No commands config file.");
    exit_err(access(world.path_interface, F_OK), "No interface config file.");

    /* Initialize the whole interface. */
    world.winDB.t_screen = initscr();
    set_cleanup_flag(CLEANUP_NCURSES);
    noecho();
    curs_set(0);
    keypad(world.winDB.t_screen, TRUE);
    init_command_db();      /* The command DB needs to be initialized before  */
    load_interface_conf();  /* the interface, whose keybindings depend on it. */

    /* Set handler for terminal window resizing. */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = &winch_called;
    exit_trouble(sigaction(SIGWINCH, &act, NULL), f_name, "sigaction()");

    /* Open file streams for communicating with the server. */
    exit_err(access(path_server_in, F_OK), "No server input file found.");
    world.file_server_in = try_fopen(path_server_in, "a", f_name);
    set_cleanup_flag(CLEANUP_SERVER_IN);
    world.file_server_out = try_fopen(path_server_out, "r", f_name);
    set_cleanup_flag(CLEANUP_SERVER_OUT);

    /* This is where most everything happens. */
    char * quit_msg = io_loop();

    /* Leave properly. */
    cleanup();
    printf("%s\n", quit_msg);
    exit(EXIT_SUCCESS);
}
