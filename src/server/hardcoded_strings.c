/* hardcoded_strings.c *
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "hardcoded_strings.h"



char * s[42];



extern void init_strings()
{
    s[S_PATH_CONFIG] = "confserver/world";
    s[S_PATH_WORLDSTATE] = "server/worldstate";
    s[S_PATH_OUT] = "server/out";
    s[S_PATH_IN] = "server/in";
    s[S_PATH_RECORD] = "record";
    s[S_PATH_SAVE] = "savefile";
    s[S_FCN_SPRINTF] = "sprintf";
    s[S_CMD_TA_ID] = "TA_ID";
    s[S_CMD_TA_EFFORT] = "TA_EFFORT";
    s[S_CMD_TA_NAME] = "TA_NAME";
    s[S_CMD_TT_ID] = "TT_ID";
    s[S_CMD_TT_CONSUM] = "TT_CONSUMABLE";
    s[S_CMD_TT_STARTN] = "TT_START_NUMBER";
    s[S_CMD_TT_HP] = "TT_LIFEPOINTS";
    s[S_CMD_TT_SYMB] = "TT_SYMBOL";
    s[S_CMD_TT_NAME] = "TT_NAME";
    s[S_CMD_TT_CORPS] = "TT_CORPSE_ID";
    s[S_CMD_TT_PROL] = "TT_PROLIFERATE";
    s[S_CMD_T_ID] = "T_ID";
    s[S_CMD_T_TYPE] = "T_TYPE";
    s[S_CMD_T_POSY] = "T_POSY";
    s[S_CMD_T_POSX] = "T_POSX";
    s[S_CMD_T_COMMAND] =  "T_COMMAND";
    s[S_CMD_T_ARGUMENT] = "T_ARGUMENT";
    s[S_CMD_T_PROGRESS] = "T_PROGRESS";
    s[S_CMD_T_HP] = "T_LIFEPOINTS";
    s[S_CMD_T_CARRIES] = "T_CARRIES";
    s[S_CMD_T_MEMMAP] = "T_MEMMAP";
    s[S_CMD_T_MEMTHING] = "T_MEMTHING";
    s[S_CMD_AI] = "ai";
    s[S_CMD_WAIT] = "wait";
    s[S_CMD_MOVE] = "move";
    s[S_CMD_PICKUP] = "pick_up";
    s[S_CMD_DROP] = "drop";
    s[S_CMD_USE] = "use";
    s[S_CMD_MAKE_WORLD] = "MAKE_WORLD";
    s[S_CMD_WORLD_ACTIVE] = "WORLD_ACTIVE";
    s[S_CMD_SEED_MAP] = "SEED_MAP";
    s[S_CMD_SEED_RAND] = "SEED_RANDOMNESS";
    s[S_CMD_TURN] = "TURN";
    s[S_CMD_MAPLENGTH] = "MAP_LENGTH";
    s[S_CMD_PLAYTYPE] = "PLAYER_TYPE";
}
