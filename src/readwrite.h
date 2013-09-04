/* readwrite.h:
 *
 * Routines for reading and writing files.
 */

#ifndef READWRITE_H
#define READWRITE_H



#include <stdio.h> /* for FILE typedef */
#include <stdint.h> /* for uint8_t, uint16_t, uint32_t */



/* Learn from "file" the largest line length (pointed to by "linemax_p"; length
 * includes newline chars) and (pointed to by "n_lines_p" if it is not set to
 * NULL) the number of lines (= number of newline chars).
 *
 * Returns 0 on success, 1 on error of fseek() (called to return to initial file
 * reading position).
 */
extern uint8_t textfile_sizes(FILE * file, uint16_t * linemax_p,
                              uint16_t * n_lines_p);



/* These routines for reading values "x" from / writing values to "file" ensure a
 * defined endianness and consistent error codes: return 0 on success and 1 on
 * fgetc()/fputc() failure.
 */
extern uint8_t read_uint8(FILE * file, uint8_t * x);
extern uint8_t read_uint16_bigendian(FILE * file, uint16_t * x);
extern uint8_t read_uint32_bigendian(FILE * file, uint32_t * x);
extern uint8_t write_uint8(uint8_t x, FILE * file);
extern uint8_t write_uint16_bigendian(uint16_t x, FILE * file);
extern uint8_t write_uint32_bigendian(uint32_t x, FILE * file);

#endif
