/* main.c */

#include <stdlib.h> /* exit() */
#include "../common/rexit.h" /* set_cleanup_func() */
#include "cleanup.h" /* cleanup() */
#include "command_db.h" /* init_command_db() */
#include "io.h" /* io_loop(), try_send() */
#include "misc.h" /* load_interface_conf() */
#include "windows.h" /* init_wmeta_and_ncurses(); */
#include "world.h" /* struct World */



struct World world;



int main()
{
    /* Declare hard-coded paths here. */
    world.path_server_in = "server/in";

    /* So error exits also go through the client's cleanup() function. */
    set_cleanup_func(cleanup);

    /* Initialize the whole interface. */
    init_wmeta_and_ncurses();
    keypad(world.wmeta.screen, TRUE);
    init_command_db();      /* The command DB needs to be initialized before  */
    load_interface_conf();  /* the interface, whose keybindings depend on it. */

    /* This is where most everything happens. */
    char * quit_msg = io_loop();

    /* Leave properly. */
    cleanup();
    printf("%s\n", quit_msg);
    exit(EXIT_SUCCESS);
}
