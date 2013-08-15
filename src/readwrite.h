/* readwrite.h:
 *
 * Routines for reading/writing (multi-)byte data from/to files. They ensure a
 * defined endianness.
 */

#ifndef READWRITE_H
#define READWRITE_H



#include <stdio.h> /* for FILE typedef */
#include <stdint.h> /* for uint8_t, uint16_t, uint32_t */



/* Each function returns 0 on success and 1 on failure. "x" is the value to be
 * written to "file" for write_* functions and for read_* functions the pointer
 * to where the value read from "file" is to be written.
 */
extern uint8_t read_uint8(FILE * file, uint8_t * x);
extern uint8_t read_uint16_bigendian(FILE * file, uint16_t * x);
extern uint8_t read_uint32_bigendian(FILE * file, uint32_t * x);
extern uint8_t write_uint8(uint8_t x, FILE * file);
extern uint8_t write_uint16_bigendian(uint16_t x, FILE * file);
extern uint8_t write_uint32_bigendian(uint32_t x, FILE * file);

#endif
