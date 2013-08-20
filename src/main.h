/* main.h
 *
 * Contains the World struct holding all game data together.
 */

#ifndef MAIN_H
#define MAIN_H



#include <stdint.h> /* for uint32_t*/
struct WinMeta;
struct Win;
struct KeyBinding;
struct KeysWinData;
struct Map;
struct ItemDef;
struct MonsterDef;



struct World
{
    char interactive;                 /* 1: playing; 0: record playback. */
    struct KeyBinding * keybindings;  /* Pointer to the keybindings. */
    struct KeysWinData * keyswindata; /* Pointer to key edit window metadata. */
    uint32_t seed;                    /* Randomness seed. */
    uint32_t turn;                    /* Current game turn. */
    char * log;                       /* Pointer to the game log string. */
    struct Map * map;                 /* Pointer to the game map cells. */
    struct ItemDef * item_def;        /* Pointer to the item definitions. */
    struct Item * item;               /* Pointer to the items' data. */
    struct MonsterDef * monster_def;  /* Pointer to the monster definitions. */
    struct Monster * monster;         /* Pointer to the monsters' data. */
    struct Player * player;           /* Pointer to the player data. */
};



#endif