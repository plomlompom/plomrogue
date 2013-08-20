/* readwrite.c */

#include "readwrite.h"
#include <stdio.h> /* for FILE typedef*/
#include <stdint.h> /* for uint8_t, uint16_t, uint32_t */



/* Read/write "x" from/to "file" as bigendian representation of "size" bits. On
 * failure, return 1, else 0. (As of of now, all extern read/write functions
 * build on top of these.)
 *
 * Only use multiples of 8 greater or equal 32 for "size". Originally a bit
 * number check prefaced the code of both functions. It was removed as redundant
 * due to all possible "size" values being hardcoded into the library (i.e. in
 * all extern functions calling / wrapping around either function). If this ever
 * changes, (re-)insert:
 *
 *    if (0 == size || size > 32 || 0 != size % 8)
 *    {
 *        return 1;
 *    }
 */
static uint8_t read_uintX_bigendian(FILE * file, uint32_t * x, uint8_t size);
static uint8_t write_uintX_bigendian(FILE * file, uint32_t x, uint8_t size);


static uint8_t read_uintX_bigendian(FILE * file, uint32_t * x, uint8_t size)
{
    * x = 0;
    int16_t bitshift = size - 8;
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
    /* Since read_uintX_bigendian() works on -- and zeroes -- four bytes, direct
     * work on values of fewer bytes would corrupt immediate neighbor values.
     */
    uint32_t y = * x;
    uint8_t err = read_uintX_bigendian(file, &y, 8);
    * x = (uint8_t) y;
    return err;
}



extern uint8_t read_uint16_bigendian(FILE * file, uint16_t * x)
{
    /* See read_uint8() introductory code comment for rationale. */
    uint32_t y = * x;
    uint8_t err = read_uintX_bigendian(file, &y, 16);
    * x = (uint16_t) y;
    return err;
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
