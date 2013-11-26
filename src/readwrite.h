/* readwrite.h:
 *
 * Routines for reading and writing files.
 */

#ifndef READWRITE_H
#define READWRITE_H

#include <stdio.h> /* for FILE typedef */
#include <stdint.h> /* for uint8_t, uint16_t, uint32_t */



/* Wrappers to fopen(), fclose(), fgets() and fwrite() from function called "f",
 * calling exit_err() upon error with appropriate error messages.
 */
extern FILE * try_fopen(char * path, char * mode, char * f);
extern void try_fclose(FILE * file, char * f);
extern void try_fwrite(void * ptr, size_t size, size_t nmemb, FILE * stream,
                       char * f);
extern void try_fputc(uint8_t c, FILE * file, char * f);

/* Wrapper to calling fgets() from function called "f". The return code of
 * fgets() is returned unless it is NULL *and* ferror() indicates that an error
 * occured; otherwise end of file is assumed and NULL is returned properly.
 */
extern char * try_fgets(char * line, int size, FILE * file, char * f);

/* Wrapper to calling fgetc() from function "f", treating either
 * (try_fgetc_noeof()) all EOF returns as errors triggering exit_trouble(), or
 * (try_fgetc()) only those accompanied by a positive ferror() result.
 */
extern int try_fgetc(FILE * file, char * f);
extern uint8_t try_fgetc_noeof(FILE * file, char * f);

/* Wrapper to successive call of fclose() from function called "f" on "file",
 * then unlink() on file at path "p2" if it exists, then rename() from path "p1"
 * to "p2". Used for handling atomic saving of files via temp files.
 */
extern void try_fclose_unlink_rename(FILE * file, char * p1, char * p2,
                                     char * f);

/* Return largest line length from "file" the largest line length (including
 * newline chars) and write the number of newline chars in "file" to the memory
 * pointed to by "n_lines_p" if it is not passed as NULL.
 */
extern uint16_t textfile_sizes(FILE * file, uint16_t * n_lines_p);

/* Read/write via try_(fgetc|fputc)() uint32 values with defined endianness. */
extern uint32_t read_uint32_bigendian(FILE * file);
extern void write_uint32_bigendian(uint32_t x, FILE * file);



#endif
