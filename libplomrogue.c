#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, INT8_MIN, INT8_MAX */



/* Coordinate for maps of max. 256x256 cells. */
struct yx_uint8
{
    uint8_t y;
    uint8_t x;
};

/* Storage for map_length, set by set_maplength(). */
static uint16_t maplength = 0;
extern void set_maplength(uint16_t maplength_input)
{
    maplength = maplength_input;
}



/* Pseudo-randomness seed for rrand(), set by seed_rrand(). */
static uint32_t seed = 0;



/* Helper to mv_yx_in_dir_legal(). Move "yx" into hex direction "d". */
static void mv_yx_in_dir(char d, struct yx_uint8 * yx)
{
    if      (d == 'e')
    {
        yx->x = yx->x + (yx->y % 2);
        yx->y--;
    }
    else if (d == 'd')
    {
        yx->x++;
    }
    else if (d == 'c')
    {
        yx->x = yx->x + (yx->y % 2);
        yx->y++;
    }
    else if (d == 'x')
    {
        yx->x = yx->x - !(yx->y % 2);
        yx->y++;
    }
    else if (d == 's')
    {
        yx->x--;
    }
    else if (d == 'w')
    {
        yx->x = yx->x - !(yx->y % 2);
        yx->y--;
    }
}



/* Move "yx" into hex direction "dir". Available hex directions are: 'e'
 * (north-east), 'd' (east), 'c' (south-east), 'x' (south-west), 's' (west), 'w'
 * (north-west). Returns 1 if the move was legal, 0 if not, and -1 when internal
 * wrapping limits were exceeded.
 *
 * A move is legal if "yx" ends up within the the map and the original wrap
 * space. The latter is left to a neighbor wrap space if "yx" moves beyond the
 * minimal (0) or maximal (UINT8_MAX) column or row of possible map space – in
 * which case "yx".y or "yx".x will snap to the respective opposite side. The
 * current wrapping state is kept between successive calls until a "yx" of NULL
 * is passed, in which case the function does nothing but zero the wrap state.
 * Successive wrapping may move "yx" several wrap spaces into either direction,
 * or return it into the original wrap space.
 */
static int8_t mv_yx_in_dir_legal(char dir, struct yx_uint8 * yx)
{
    static int8_t wrap_west_east   = 0;
    static int8_t wrap_north_south = 0;
    if (!yx)
    {
        wrap_west_east = wrap_north_south = 0;
        return 0;
    }
    if (   INT8_MIN == wrap_west_east || INT8_MIN == wrap_north_south
        || INT8_MAX == wrap_west_east || INT8_MAX == wrap_north_south)
    {
        return -1;
    }
    struct yx_uint8 original = *yx;
    mv_yx_in_dir(dir, yx);
    if      (('e' == dir || 'd' == dir || 'c' == dir) && yx->x < original.x)
    {
        wrap_west_east++;
    }
    else if (('x' == dir || 's' == dir || 'w' == dir) && yx->x > original.x)
    {
        wrap_west_east--;
    }
    if      (('w' == dir || 'e' == dir)               && yx->y > original.y)
    {
        wrap_north_south--;
    }
    else if (('x' == dir || 'c' == dir)               && yx->y < original.y)
    {
        wrap_north_south++;
    }
    if (   !wrap_west_east && !wrap_north_south
        && yx->x < maplength && yx->y < maplength)
    {
        return 1;
    }
    return 0;
}



/* Wrapper around mv_yx_in_dir_legal() that stores new coordinate in res_y/x,
 * (return with result_y/x()), and immediately resets the wrapping.
 */
static uint8_t res_y = 0;
static uint8_t res_x = 0;
extern uint8_t mv_yx_in_dir_legal_wrap(char dir, uint8_t y, uint8_t x)
{
    struct yx_uint8 yx;
    yx.y = y;
    yx.x = x;
    uint8_t result = mv_yx_in_dir_legal(dir, &yx);
    mv_yx_in_dir_legal(0, NULL);
    res_y = yx.y;
    res_x = yx.x;
    return result;
}
extern uint8_t result_y()
{
    return res_y;
}
extern uint8_t result_x()
{
    return res_x;
}



/* With set_seed set, set seed global to seed_input. In any case, return it. */
extern uint32_t seed_rrand(uint8_t set_seed, uint32_t seed_input)
{
    if (set_seed)
    {
        seed = seed_input;
    }
    return seed;
}



/* Return 16-bit number pseudo-randomly generated via Linear Congruential
 * Generator algorithm with some proven constants. Use instead of any rand() to
  * ensure portability of the same pseudo-randomness across systems.
 */
extern uint16_t rrand()
{   /* Constants as recommended by POSIX.1-2001 (see man page rand(3)). */
    seed = ((seed * 1103515245) + 12345) % 4294967296;
    return (seed >> 16); /* Ignore less random least significant bits. */
}
