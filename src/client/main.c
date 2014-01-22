/* main.c */

#include <signal.h> /* struct sigaction, sigaction() */
#include <stdlib.h> /* exit() */
#include <string.h> /* memset() */
#include "../common/rexit.h" /* set_cleanup_func(), exit_trouble() */
#include "cleanup.h" /* cleanup() */
#include "command_db.h" /* init_command_db() */
#include "io.h" /* io_loop(), try_send() */
#include "misc.h" /* load_interface_conf(), winch_called() */
#include "windows.h" /* init_wmeta_and_ncurses(); */
#include "world.h" /* struct World */



struct World world;



int main()
{
    char * f_name = "main()";

    /* Declare hard-coded paths here. */
    world.path_server_in = "server/in";

    /* So error exits also go through the client's cleanup() function. */
    set_cleanup_func(cleanup);

    /* Initialize the whole interface. */
    init_wmeta_and_ncurses();
    keypad(world.wmeta.screen, TRUE);
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
