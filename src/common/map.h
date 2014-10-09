/* src/common/map.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
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
