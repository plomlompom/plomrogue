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
struct KeyBinding;
struct KeysWinData;
struct Map;
struct ItemDef;
struct MonsterDef;



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
    struct ItemDef * item_def;        /* Pointer to the item definitions. */
    struct Item * item;               /* Pointer to the items' data. */
    struct MonsterDef * monster_def;  /* Pointer to the monster definitions. */
    struct Monster * monster;         /* Pointer to the monsters' data. */
    struct Player * player;           /* Pointer to the player data. */
    struct CommandDB * cmd_db;        /* Pointer to the command database. */
    struct WinMeta * wmeta;           /* Pointer to window manager's WinMeta. */
    struct WinConf * winconfs;        /* Pointer to windows' configurations. */
    char * winconf_ids;               /* Pointer to string of Winconfs' ids. */
    uint8_t map_object_count;         /* Counts loaded/generated map objects. */
};



#endif
