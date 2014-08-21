/* src/server/map.c */

#include "map.h"
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT16_MAX */
#include <stdlib.h> /* free() */
#include "../common/rexit.h" /* exit_err() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "../common/yx_uint8.h" /* struct yx_uint8 */
#include "rrand.h" /* rrand() */
#include "yx_uint8.h" /* mv_yx_in_dir_wrap() */
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
    uint8_t diag_east = pos.x + ind <= world.map.length - 1;
    uint16_t pos_i = (pos.y * world.map.length) + pos.x;
    if (   (   pos.y > 0                    && diag_east
            && type == world.map.cells[pos_i - world.map.length + ind])
        || (   pos.x < world.map.length - 1
            && type == world.map.cells[pos_i + 1])
        || (   pos.y < world.map.length - 1 && diag_east
            && type == world.map.cells[pos_i + world.map.length + ind])
        || (   pos.y > 0                    && diag_west
            && type == world.map.cells[pos_i - world.map.length - !ind])
        || (   pos.x > 0
            && type == world.map.cells[pos_i - 1])
        || (   pos.y < world.map.length - 1 && diag_west
            && type == world.map.cells[pos_i + world.map.length - !ind]))
    {
        return 1;
    }
    return 0;
}



static void make_sea()
{
    uint16_t y, x;
    for (y = 0; y < world.map.length; y++)
    {
        for (x = 0;
             x < world.map.length;
             world.map.cells[(y * world.map.length) + x] = '~', x++);
    }
}



static void make_island()
{
    char type = '.';
    uint8_t add_half_width = !(world.map.length % 2) * (world.map.length / 2);
    uint32_t size = world.map.length * world.map.length;
    world.map.cells[(size / 2) + add_half_width] = type;
    struct yx_uint8 pos;
    iter_limit(1);
    while (iter_limit(0))
    {
        pos.y = rrand() % world.map.length;
        pos.x = rrand() % world.map.length;
        uint16_t pos_i = (pos.y * world.map.length) + pos.x;
        if ('~' == world.map.cells[pos_i] && is_neighbor(pos, type))
        {
            if (   pos.y == 0 || pos.y == world.map.length - 1
                || pos.x == 0 || pos.x == world.map.length - 1)
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
    uint16_t n_trees = (world.map.length * world.map.length) / 16;
    uint16_t i_trees = 0;
    iter_limit(1);
    while (i_trees <= n_trees && iter_limit(0))
    {
        uint8_t single_allowed = rrand() % 32;
        pos.y = rrand() % world.map.length;
        pos.x = rrand() % world.map.length;
        uint16_t pos_i = (pos.y * world.map.length) + pos.x;
        if ('.' == world.map.cells[pos_i]
            && (!single_allowed || is_neighbor(pos, type)))
        {
            world.map.cells[pos_i] = type;
            i_trees++;
        }
    }
}



extern void remake_map()
{
    free(world.map.cells);
    world.map.cells = try_malloc(world.map.length * world.map.length, __func__);
    uint32_t store_seed = world.seed;
    world.seed = world.seed_map;
    make_sea();
    make_island();
    make_trees();
    world.seed = store_seed;
}



extern uint8_t mv_yx_in_dir_legal(char dir, struct yx_uint8 * yx)
{
    uint8_t wraptest = mv_yx_in_dir_wrap(dir, yx, 0);
    if (!wraptest && yx->x < world.map.length && yx->y < world.map.length)
    {
        return 1;
    }
    return 0;
}
