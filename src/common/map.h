/* src/common/map.h
 *
 * Game map.
 */

#ifndef MAP_H
#define MAP_H

#include <stdint.h> /* uint16_t */



struct Map
{
    char * cells;            /* sequence of bytes encoding map cells */
    uint16_t length;         /* map's edge length, i.e. both height and width */
};



#endif
