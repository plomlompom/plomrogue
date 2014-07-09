/* hardcoded_strings.c */

#include "hardcoded_strings.h"



char * s[26];



extern void init_strings()
{
    s[S_PATH_CONFIG] = "confserver/world";
    s[S_PATH_WORLDSTATE] = "server/worldstate";
    s[S_PATH_OUT] = "server/out";
    s[S_PATH_IN] = "server/in";
    s[S_PATH_RECORD] = "record";
    s[S_PATH_SAVE] = "savefile";
    s[S_CMD_MAKE_WORLD] = "MAKE_WORLD";
    s[S_CMD_DO_FOV] = "BUILD_FOVS";
    s[S_CMD_SEED_MAP] = "SEED_MAP";
    s[S_CMD_SEED_RAND] = "SEED_RANDOMNESS";
    s[S_CMD_TURN] = "TURN";
    s[S_CMD_THING] = "THING";
    s[S_CMD_TYPE] = "TYPE";
    s[S_CMD_POS_Y] = "POS_Y";
    s[S_CMD_POS_X] = "POS_X";
    s[S_CMD_COMMAND] =  "COMMAND";
    s[S_CMD_ARGUMENT] = "ARGUMENT";
    s[S_CMD_PROGRESS] = "PROGRESS";
    s[S_CMD_LIFEPOINTS] = "LIFEPOINTS";
    s[S_CMD_CARRIES] = "CARRIES";
    s[S_CMD_WAIT] = "wait";
    s[S_CMD_MOVE] = "move";
    s[S_CMD_PICKUP] = "pick_up";
    s[S_CMD_DROP] = "drop";
    s[S_CMD_USE] = "use";
    s[S_FCN_SPRINTF] = "sprintf()";
}
