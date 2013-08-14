/* readwrite.c */

#include "readwrite.h"
#include <stdio.h> /* for FILE typedef*/
#include <stdint.h> /* for uint16_t, uint32_t */



/* Read/write "x" from/to "file" as bigendian representation of "size" bits. */
static uint8_t read_uintX_bigendian(FILE * file, uint32_t * x, uint8_t size);
static uint8_t write_uintX_bigendian(FILE * file, uint32_t x, uint8_t size);



static uint8_t read_uintX_bigendian(FILE * file, uint32_t * x, uint8_t size)
{
    if (0 != size % 8)
    {
        return 1;
    }
    int16_t bitshift = size - 8;

    * x = 0;
    int test;
    for (; bitshift >= 0; bitshift = bitshift - 8)
    {
        test = fgetc(file);
        if (EOF == test)
        {
            return 1;
        }
        * x = * x + ((uint32_t) test << bitshift);
    }
    return 0;
}



static uint8_t write_uintX_bigendian(FILE * file, uint32_t x, uint8_t size)
{
    if (0 != size % 8)
    {
        return 1;
    }
    int16_t bitshift = size - 8;

    for (; bitshift >= 0; bitshift = bitshift - 8)
    {
        if (EOF == fputc((x >> bitshift) & 0xFF, file))
        {
            return 1;
        }
    }
    return 0;
}



extern uint8_t read_uint8(FILE * file, uint8_t * x)
{
    uint32_t y = * x;
    uint8_t fail = read_uintX_bigendian(file, &y, 8);
    * x = (uint8_t) y;
    return fail;
}



extern uint8_t read_uint16_bigendian(FILE * file, uint16_t * x)
{
    uint32_t y = * x;
    uint8_t fail = read_uintX_bigendian(file, &y, 16);
    * x = (uint16_t) y;
    return fail;
}



extern uint8_t read_uint32_bigendian(FILE * file, uint32_t * x)
{
    return read_uintX_bigendian(file, x, 32);
}



extern uint8_t write_uint8(uint8_t x, FILE * file)
{
    return write_uintX_bigendian(file, x, 8);
}



extern uint8_t write_uint16_bigendian(uint16_t x, FILE * file)
{
    return write_uintX_bigendian(file, x, 16);
}



extern uint8_t write_uint32_bigendian(uint32_t x, FILE * file)
{
    return write_uintX_bigendian(file, x, 32);
}
