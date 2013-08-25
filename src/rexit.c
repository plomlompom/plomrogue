/* rexit.c */

#include "rexit.h"
#include <stdlib.h> /* for exit(), free(), defines EXIT_SUCESS, EXIT_FAILURE */
#include <stdio.h> /* for printf(), perror() */
#include <stdint.h> /* for uint8_t */
#include <ncurses.h> /* for endwin() */
#include "main.h" /* for World struct */
#include "map.h" /* for Map struct */
#include "keybindings.h" /* for KeysWinData, KeyBinding structs */


/* The clean-up routine and the flag resource by which it decides what to do. */
static uint8_t cleanup_flags = 0x00;
static void cleanup(struct World * world);



static void cleanup(struct World * world)
{
    if (cleanup_flags & CLEANUP_NCURSES)
    {
        endwin();
    }
    if (cleanup_flags & CLEANUP_MAP)
    {
        free(world->map->cells);
    }
    if (cleanup_flags & CLEANUP_KEYBINDINGS)
    {
        uint16_t key;
        for (key = 0; key <= world->keyswindata->max; key++)
        {
            free(world->keybindings[key].name);
        }
        free(world->keybindings);
        free(world->keyswindata);
    }
    if (cleanup_flags & CLEANUP_LOG)
    {
        free(world->log);
    }
}



extern void set_cleanup_flag(enum cleanup_flag flag)
{
    cleanup_flags = cleanup_flags | flag;
}



extern void exit_game(struct World * world)
{
    cleanup(world);
    exit(EXIT_SUCCESS);
}



extern void exit_err(uint8_t err, struct World * world, char * msg)
{
    if (0 == err)
    {
        return;
    }
    cleanup(world);
    if (NULL == msg)
    {
        msg = "Details unknown.";
    }
    printf("Aborted PlomRogue due to error. %s\n", msg);
    perror("errno states");
    exit(EXIT_FAILURE);
}
