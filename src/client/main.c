/* main.c */

#include <ncurses.h> /* keypad() */
#include <signal.h> /* struct sigaction, sigaction() */
#include <stddef.h> /* NULL */
#include <stdlib.h> /* exit() */
#include <string.h> /* memset() */
#include <unistd.h> /* access() */
#include "../common/rexit.h" /* set_cleanup_func(), exit_trouble(),exit_err() */
#include "cleanup.h" /* cleanup(), set_cleanup_flag() */
#include "command_db.h" /* init_command_db() */
#include "io.h" /* io_loop(), try_send() */
#include "misc.h" /* load_interface_conf(), winch_called() */
#include "windows.h" /* winch_called() */
#include "world.h" /* struct World */



struct World world;



int main(int argc, char * argv[])
{
    char * f_name = "main()";

    /* Declare hard-coded paths and values here. */
    world.path_server_in  = "server/in";
    world.path_commands   = "confclient/commands";
    world.path_interface  = "confclient/interface_conf";
    world.winDB.legal_ids = "012ciklm";
    world.delim           = "%\n";

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

    /* This is where most everything happens. */
    char * quit_msg = io_loop();

    /* Leave properly. */
    cleanup();
    printf("%s\n", quit_msg);
    exit(EXIT_SUCCESS);
}
