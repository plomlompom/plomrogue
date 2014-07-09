/* hardcoded_strings.h
 *
 * For re-used hardcoded strings.
 */

#ifndef STRINGS_H
#define STRINGS_H



enum string_num
{
    S_S_PATH_CONFIG,
    S_S_PATH_WORLDSTATE,
    S_S_PATH_OUT,
    S_S_PATH_IN,
    S_S_PATH_RECORD,
    S_S_PATH_SUFFIX_TMP,
    S_S_PATH_SAVE,
    S_S_CMD_MAKE_WORLD,
    S_S_CMD_DO_FOV,
    S_S_CMD_SEED_MAP,
    S_S_CMD_SEED_RAND,
    S_S_CMD_TURN,
    S_S_CMD_THING,
    S_S_CMD_TYPE,
    S_S_CMD_POS_Y,
    S_S_CMD_POS_X,
    S_S_CMD_COMMAND,
    S_S_CMD_ARGUMENT,
    S_S_CMD_PROGRESS,
    S_S_CMD_LIFEPOINTS,
    S_S_CMD_CARRIES,
    S_S_CMD_WAIT,
    S_S_CMD_MOVE,
    S_S_CMD_PICKUP,
    S_S_CMD_DROP,
    S_S_CMD_USE
};

extern void init_strings();

extern char * s[26];



#endif
