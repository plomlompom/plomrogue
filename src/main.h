/* main.h
 *
 * Contains the World struct holding all game data together.
 */

#ifndef MAIN_H
#define MAIN_H



#include <stdint.h> /* for uint32_t*/
#include "keybindings.h"
struct WinMeta;
struct WinConf;
struct Win;
struct Map;
struct MapObjDef;
struct MapObj;



struct World
{
    char interactive;                 /* 1: playing; 0: record playback. */
    struct KeyBiData kb_global;       /* Global keybindings. */
    struct KeyBiData kb_wingeom;      /* Window geometry config keybindings. */
    struct KeyBiData kb_winkeys;      /* Window keybinding config keybindings.*/
    uint32_t seed;                    /* Randomness seed. */
    uint32_t turn;                    /* Current game turn. */
    uint16_t score;                   /* Player's score. */
    char * log;                       /* Pointer to the game log string. */
    struct Map * map;                 /* Pointer to the game map cells. */
    struct CommandDB * cmd_db;        /* Pointer to the command database. */
    struct WinMeta * wmeta;           /* Pointer to window manager's WinMeta. */
    struct WinConf * winconfs;        /* Pointer to windows' configurations. */
    char * winconf_ids;               /* Pointer to string of Winconfs' ids. */
    uint8_t map_obj_count;            /* Counts map objects generated so far. */
    struct MapObjDef * map_obj_defs;  /* Map object type definitions chain. */
    struct MapObj * map_objs;         /* Pointer to map objects chain start. */
    uint8_t inventory_select;         /* Player's inventory selection index. */
    uint8_t old_inventory_select;     /* Temporarily stores for recordfile */
};                                    /* writing inventory selection index. */



#endif
