/* src/server/map.c */

#include "map.h"
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* struct yx_uint8 */
#include "rrand.h" /* rrand() */
#include "world.h" /* global world */



/* Call this too often with "init" of 0 and the game exits with an error message
 * about reaching an iteration limit. An "init" of 1 sets the iteration counter
 * to 0. Iteration limit is currently 256 * UINT16_MAX.
 */
static uint8_t iter_limit(uint8_t init);

/* Return 1 if cell on "pos" is neighbor to a cell of "type", else return 0. */
static uint8_t is_neighbor(struct yx_uint8 pos, char type);

/* Fill map with '~' cells. */
static void make_sea();

/* Put island of '.' cells inside map sea. */
static void make_sea();

/* Put tree cells of 'X' on island. */
static void make_trees();



static uint8_t iter_limit(uint8_t init)
{
    static uint32_t i = 0;
    char * err = "Map generation reached iteration limit. Change map size?";
    if (init)
    {
        i = 0;
        return 0;
    }
    i++;
    exit_err(256 * UINT16_MAX == i, err);
    return 1;
}



static uint8_t is_neighbor(struct yx_uint8 pos, char type)
{
    uint8_t ind = pos.y % 2;
    uint8_t diag_west = pos.x + ind > 0;
    uint8_t diag_east = pos.x + ind <= world.map.size.x - 1;
    uint16_t pos_i = (pos.y * world.map.size.x) + pos.x;
    if (   (   pos.y > 0                    && diag_east
            && type == world.map.cells[pos_i - world.map.size.x + ind])
        || (   pos.x < world.map.size.x - 1
            && type == world.map.cells[pos_i + 1])
        || (   pos.y < world.map.size.y - 1 && diag_east
            && type == world.map.cells[pos_i + world.map.size.x + ind])
        || (   pos.y > 0                    && diag_west
            && type == world.map.cells[pos_i - world.map.size.x - !ind])
        || (   pos.x > 0
            && type == world.map.cells[pos_i - 1])
        || (   pos.y < world.map.size.y - 1 && diag_west
            && type == world.map.cells[pos_i + world.map.size.x - !ind]))
    {
        return 1;
    }
    return 0;
}



static void make_sea()
{
    uint16_t y, x;
    for (y = 0; y < world.map.size.y; y++)
    {
        for (x = 0;
             x < world.map.size.x;
             world.map.cells[(y * world.map.size.x) + x] = '~', x++);
    }
}



static void make_island()
{
    char type = '.';
    uint8_t add_half_width = !(world.map.size.y % 2) * (world.map.size.x / 2);
    uint32_t size = world.map.size.x * world.map.size.y;
    world.map.cells[(size / 2) + add_half_width] = type;
    struct yx_uint8 pos;
    iter_limit(1);
    while (iter_limit(0))
    {
        pos.y = rrand() % world.map.size.y;
        pos.x = rrand() % world.map.size.x;
        uint16_t pos_i = (pos.y * world.map.size.x) + pos.x;
        if ('~' == world.map.cells[pos_i] && is_neighbor(pos, type))
        {
            if (   pos.y == 0 || pos.y == world.map.size.y - 1
                || pos.x == 0 || pos.x == world.map.size.x - 1)
            {
                break;
            }
            world.map.cells[pos_i] = type;
        }
    }
}



static void make_trees()
{
    char type = 'X';
    struct yx_uint8 pos;
    uint16_t n_trees = (world.map.size.x * world.map.size.y) / 16;
    uint16_t i_trees = 0;
    iter_limit(1);
    while (i_trees <= n_trees && iter_limit(0))
    {
        uint8_t single_allowed = rrand() % 32;
        pos.y = rrand() % world.map.size.y;
        pos.x = rrand() % world.map.size.x;
        uint16_t pos_i = (pos.y * world.map.size.x) + pos.x;
        if ('.' == world.map.cells[pos_i]
            && (!single_allowed || is_neighbor(pos, type)))
        {
            world.map.cells[pos_i] = type;
            i_trees++;
        }
    }
}



extern void init_map()
{
    char * f_name = "init_map()";
    world.map.cells = try_malloc(world.map.size.x * world.map.size.y, f_name);
    make_sea();
    make_island();
    make_trees();
}



extern uint8_t is_passable(struct yx_uint8 pos)
{
    uint8_t passable = 0;
    if (pos.x < world.map.size.x && pos.y < world.map.size.y)
    {
        passable = ('.' == world.map.cells[(pos.y * world.map.size.x) + pos.x]);
    }
    return passable;
}
