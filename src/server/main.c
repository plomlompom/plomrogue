/* src/server/main.c */

#include <stdio.h> /* printf() */
#include <stdlib.h> /* exit() */
#include "../common/rexit.h" /* exit_err, set_cleanup_func() */
#include "cleanup.h" /* set_cleanup_flag(), cleanup() */
#include "configfile.h" /* read_config_file() */
#include "hardcoded_strings.h" /* s */
#include "init.h" /* run_game(), obey_argv(), obey_argv(), setup_server_io() */
#include "world.h" /* struct World */



struct World world;



int main(int argc, char ** argv)
{
    /* So error exits also go through the server's cleanup() function. */
    set_cleanup_func(cleanup);

    /* Init settings from command line / hard-coded values. Print start info. */
    init_strings();
    obey_argv(argc, argv);
    if (world.is_verbose)
    {
        char * printf_err = "Trouble in main() with printf().";
        int test = printf("Starting plomrogue-server.\n");
        exit_err(-1 == test, printf_err);
        if (world.replay)
        {
            test = printf("Replay mode. Auto-replaying up to turn %d.\n",
                         world.replay);
            exit_err(-1 == test, printf_err);
        }
    }

    /* Init config file and server i/o files. */
    read_config_file();
    setup_server_io();

    /* Enter play or replay mode loops, then leave properly. */
    run_game();
    cleanup();
    exit(EXIT_SUCCESS);
}
