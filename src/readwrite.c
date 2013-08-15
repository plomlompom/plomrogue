/* readwrite.c */

#include "readwrite.h"
#include <stdio.h> /* for FILE typedef*/
#include <stdint.h> /* for uint16_t, uint32_t */



/* Read/write "x" from/to "file" as bigendian representation of "size" bits.
 * Only multiples of 8 allowed for "size". On failure, return 1, else 0.
 *
 * As of of now, all other read/write functions build on top of these. TODO:
 * Consider externing these so-far internal functions and dropping the
 * interfaces to them, instead relying on their internal validity checks.
 * (Usage of padded memory copies instead of directly manipulating * x as is
 * done in read_uint* would need to be added, though.)
 */
static uint8_t read_uintX_bigendian(FILE * file, uint32_t * x, uint8_t size);
static uint8_t write_uintX_bigendian(FILE * file, uint32_t x, uint8_t size);



static uint8_t read_uintX_bigendian(FILE * file, uint32_t * x, uint8_t size)
{
    if (0 != size % 8)           /* This bit number validity check is redundant */
    {                            /* as long as this function is only available  */
        return 1;                /* through extern interfaces pre-defining the  */
    }                            /* bit number. TODO: Consider dropping it.     */
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
    if (0 != size % 8)                             /* See comment             */
    {                                              /* on identical            */
        return 1;                                  /* code block in           */
    }                                              /* read_uintX_bigendian(). */
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
    /* See read_uint8() introductory comment for rationale. */
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
