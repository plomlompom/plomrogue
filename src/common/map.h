/* src/common/map.h
 *
 * Game map.
 */

#ifndef MAP_H
#define MAP_H

#include "yx_uint16.h" /* yx_uint16 struct */


struct Map
{
    struct yx_uint16 size;   /* map's height/width in number of cells */
    char * cells;            /* sequence of bytes encoding map cells */
};



#endif
