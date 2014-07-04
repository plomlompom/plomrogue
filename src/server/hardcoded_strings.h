/* hardcoded_strings.h
 *
 * For re-used hardcoded strings.
 */

#ifndef STRINGS_H
#define STRINGS_H



enum string_num
{
    PATH_CONFIG,
    PATH_WORLDSTATE,
    PATH_OUT,
    PATH_IN,
    PATH_RECORD,
    PATH_SUFFIX_TMP,
    PATH_SAVE,
    CMD_MAKE_WORLD,
    CMD_DO_FOV,
    CMD_SEED_MAP,
    CMD_SEED_RAND,
    CMD_TURN,
    CMD_THING,
    CMD_TYPE,
    CMD_POS_Y,
    CMD_POS_X,
    CMD_COMMAND,
    CMD_ARGUMENT,
    CMD_PROGRESS,
    CMD_LIFEPOINTS,
    CMD_CARRIES,
    CMD_WAIT,
    CMD_MOVE,
    CMD_PICKUP,
    CMD_DROP,
    CMD_USE
};

extern void init_strings();

extern char * s[26];



#endif
