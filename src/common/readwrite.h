/* src/common/readwrite.h:
 *
 * Routines for reading and writing files.
 */

#ifndef READWRITE_H
#define READWRITE_H

#include <stdint.h> /* uint8_t, uint32_t */
#include <stdio.h> /* FILE */



/* Wrappers to fopen(), fclose(), fgets() and fwrite() from function called "f",
 * calling exit_err() upon error with appropriate error messages.
 */
extern FILE * try_fopen(char * path, char * mode, char * f);
extern void try_fclose(FILE * file, char * f);
extern void try_fwrite(void * ptr, size_t size, size_t nmemb, FILE * stream,
                       char * f);
extern void try_fputc(uint8_t c, FILE * file, char * f);

/* Wrapper to calling fgetc() and fgets() from function "f". The return code is
 * returned unless ferror() indicates an error (i.e. to signify an end of file,
 * fgetc() may return an EOF and fgets() a NULL). try_fgetc() calls clearerr()
 * on "file" before fgetc(), because some Unixes fgetc() remember old EOFs and
 * only return those until explicitely cleared.
 */
extern int try_fgetc(FILE * file, char * f);
extern char * try_fgets(char * line, int size, FILE * file, char * f);

/* Wrapper to successive call of fclose() from function called "f" on "file",
 * then unlink() on file at path "p2" if it exists, then rename() from path "p1"
 * to "p2". Used for handling atomic saving of files via temp files.
 */
extern void try_fclose_unlink_rename(FILE * file, char * p1, char * p2,
                                     char * f);

/* Return largest line length from "file" (including  newline chars). */
extern uint32_t textfile_width(FILE * file);



#endif
