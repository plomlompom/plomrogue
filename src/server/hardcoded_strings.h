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
    S_PATH_SAVE,
    S_CMD_MAKE_WORLD,
    S_CMD_WORLD_ACTIVE,
    S_CMD_SEED_MAP,
    S_CMD_SEED_RAND,
    S_CMD_TURN,
    S_CMD_THING,
    S_CMD_T_TYPE,
    S_CMD_T_POSY,
    S_CMD_T_POSX,
    S_CMD_T_COMMAND,
    S_CMD_T_ARGUMENT,
    S_CMD_T_PROGRESS,
    S_CMD_T_HP,
    S_CMD_T_CARRIES,
    S_CMD_WAIT,
    S_CMD_MOVE,
    S_CMD_PICKUP,
    S_CMD_DROP,
    S_CMD_USE,
    S_FCN_SPRINTF,
    S_CMD_THINGTYPE,
    S_CMD_TT_CONSUM,
    S_CMD_TT_STARTN,
    S_CMD_TT_HP,
    S_CMD_TT_SYMB,
    S_CMD_TT_NAME,
    S_CMD_TT_CORPS,
    S_CMD_THINGACTION,
    S_CMD_TA_EFFORT,
    S_CMD_TA_NAME,
    S_CMD_MAPLENGTH,
    S_CMD_PLAYTYPE
};

extern void init_strings();

extern char * s[38];



#endif
