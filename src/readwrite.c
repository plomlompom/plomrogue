/* readwrite.c */

#include "readwrite.h"
#include <stdio.h>
#include <stdint.h>

extern uint16_t read_uint16_bigendian(FILE * file)
{
    uint16_t x;
    x =     (uint16_t) fgetc(file) << 8;
    x = x + (uint16_t) fgetc(file);
    return x;
}

extern uint32_t read_uint32_bigendian(FILE * file)
{
    uint32_t x;
    x =       (uint32_t) fgetc(file) << 24;
    x = x + ( (uint32_t) fgetc(file) << 16 );
    x = x + ( (uint32_t) fgetc(file) <<  8 );
    x = x +   (uint32_t) fgetc(file);
    return x;
}

extern void write_uint16_bigendian(uint16_t x, FILE * file)
{
    fputc( x >> 8,   file );
    fputc( x & 0xFF, file );
}

extern void write_uint32_bigendian(uint32_t x, FILE * file)
{
    fputc(   x >> 24,          file);
    fputc( ( x >> 16 ) & 0xFF, file);
    fputc( ( x >>  8 ) & 0xFF, file);
    fputc(   x         & 0xFF, file);
}
