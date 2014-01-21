/* src/client/world.h
 *
 * Contains the World struct holding all quasi-global game data together.
 */

#ifndef WORLD_H
#define WORLD_H

#include <stdint.h> /* uint8_t, uint16_t */
#include <sys/types.h> /* time_t */
#include "../common/map.h" /* struct Map */
#include "../common/yx_uint16.h" /* struct yx_uint16 */
#include "keybindings.h" /* stuct KeyBindingDB */
#include "command_db.h" /* struct CommandDB */
#include "windows.h" /* struct WinMeta */
#include "wincontrol.h" /* WinConfDB */



struct World
{
    struct WinMeta wmeta;
    struct WinConfDB winconf_db;
    struct CommandDB cmd_db;        /* Command database. */
    struct KeyBindingDB kb_global;    /* Global keybindings. */
    struct KeyBindingDB kb_wingeom;   /* Window geometry config keybindings. */
    struct KeyBindingDB kb_winkeys; /* Window keybinding config keybindings.*/
    struct Map map;                 /* Pointer to the game map cells. */
    time_t last_update;
    struct yx_uint16 player_pos;
    char * log;
    char * path_server_in;
    char * player_inventory;
    uint16_t turn;
    uint16_t score;
    uint8_t halfdelay;
    uint8_t player_inventory_select;
    uint8_t player_lifepoints;
};



extern struct World world;



#endif
