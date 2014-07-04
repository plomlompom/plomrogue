/* hardcoded_strings.c */

#include "hardcoded_strings.h"



char * s[26];



extern void init_strings()
{
    s[PATH_CONFIG] = "confserver/world";
    s[PATH_WORLDSTATE] = "server/worldstate";
    s[PATH_OUT] = "server/out";
    s[PATH_IN] = "server/in";
    s[PATH_RECORD] = "record";
    s[PATH_SUFFIX_TMP] = "_tmp";
    s[PATH_SAVE] = "savefile";
    s[CMD_MAKE_WORLD] = "MAKE_WORLD";
    s[CMD_DO_FOV] = "BUILD_FOVS";
    s[CMD_SEED_MAP] = "SEED_MAP";
    s[CMD_SEED_RAND] = "SEED_RANDOMNESS";
    s[CMD_TURN] = "TURN";
    s[CMD_THING] = "THING";
    s[CMD_TYPE] = "TYPE";
    s[CMD_POS_Y] = "POS_Y";
    s[CMD_POS_X] = "POS_X";
    s[CMD_COMMAND] =  "COMMAND";
    s[CMD_ARGUMENT] = "ARGUMENT";
    s[CMD_PROGRESS] = "PROGRESS";
    s[CMD_LIFEPOINTS] = "LIFEPOINTS";
    s[CMD_CARRIES] = "CARRIES";
    s[CMD_WAIT] = "wait";
    s[CMD_MOVE] = "move";
    s[CMD_PICKUP] = "pick_up";
    s[CMD_DROP] = "drop";
    s[CMD_USE] = "use";
}
