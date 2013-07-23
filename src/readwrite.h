/* readwrite.h:
 *
 * Routines for reading/writing multibyte data from/to files. They ensure a
 * defined endianness.
 */

#ifndef READWRITE_H
#define READWRITE_H



#include <stdio.h> /* for FILE typedef */
#include <stdint.h> /* for uint16_t, uint32_t */



extern uint16_t read_uint16_bigendian(FILE * file);
extern uint32_t read_uint32_bigendian(FILE * file);
extern void write_uint16_bigendian(uint16_t x, FILE * file);
extern void write_uint32_bigendian(uint32_t x, FILE * file);

#endif
