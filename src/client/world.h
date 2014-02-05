/* src/client/world.h
 *
 * Contains the World struct holding all quasi-global game data together.
 */

#ifndef WORLD_H
#define WORLD_H

#include <stdint.h> /* uint8_t, uint16_t */
#include <sys/types.h> /* time_t */
#include "map.h" /* struct Map */
#include "../common/yx_uint16.h" /* struct yx_uint16 */
#include "keybindings.h" /* stuct KeyBindingDB */
#include "command_db.h" /* struct CommandDB */
#include "windows.h" /* WinDB */



struct World
{
    struct WinDB winDB; /* data for window management and individual windows */
    struct CommandDB commandDB; /* data on commands from commands config file */
    struct KeyBindingDB kb_global; /* globally availabe keybindings */
    struct KeyBindingDB kb_wingeom; /* Win geometry config view keybindings */
    struct KeyBindingDB kb_winkeys; /* Win keybindings config view keybindings*/
    struct Map map; /* game map geometry and content */
    time_t last_update; /* used for comparison with server outfile' mtime */
    struct yx_uint16 player_pos; /* coordinates of player on map */
    char * log; /* log of player's activities */
    char * path_server_in; /* path of server's input fifo */
    char * path_interface; /* path of interface configuration file */
    char * path_commands; /* path of commands config file */
    char * player_inventory; /* one-item-per-line string list of owned items */
    char * delim; /* delimiter for multi-line blocks in config files */
    uint16_t turn; /* world/game turn */
    uint16_t player_score; /* player's score*/
    uint8_t halfdelay; /* how long to wait for getch() input in io_loop() */
    uint8_t player_inventory_select; /* index of selected item in inventory */
    uint8_t player_lifepoints; /* how alive the player is */
    uint8_t winch; /* if set, SIGWINCH was registered; trigger reset_windows()*/
};



extern struct World world;



#endif
