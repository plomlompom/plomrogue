/* hardcoded_strings.h
 *
 * For re-used hardcoded strings.
 */

#ifndef STRINGS_H
#define STRINGS_H



enum string_num
{
    S_PATH_CONFIG,
    S_PATH_WORLDSTATE,
    S_PATH_OUT,
    S_PATH_IN,
    S_PATH_RECORD,
    S_PATH_SUFFIX_TMP,
    S_PATH_SAVE,
    S_CMD_MAKE_WORLD,
    S_CMD_DO_FOV,
    S_CMD_SEED_MAP,
    S_CMD_SEED_RAND,
    S_CMD_TURN,
    S_CMD_THING,
    S_CMD_TYPE,
    S_CMD_POS_Y,
    S_CMD_POS_X,
    S_CMD_COMMAND,
    S_CMD_ARGUMENT,
    S_CMD_PROGRESS,
    S_CMD_LIFEPOINTS,
    S_CMD_CARRIES,
    S_CMD_WAIT,
    S_CMD_MOVE,
    S_CMD_PICKUP,
    S_CMD_DROP,
    S_CMD_USE
};

extern void init_strings();

extern char * s[26];



#endif
