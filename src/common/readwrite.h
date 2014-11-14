/* src/common/readwrite.h:
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
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
extern FILE * try_fopen(char * path, char * mode, const char * f);
extern void try_fclose(FILE * file, const char * f);
extern void try_fwrite(void * ptr, size_t size, size_t nmemb, FILE * stream,
                       const char * f);
extern void try_fputc(uint8_t c, FILE * file, const char * f);

/* Wrapper to calling fgetc() and fgets() from function "f". The return code is
 * returned unless ferror() indicates an error (i.e. to signify an end of file,
 * fgetc() may return an EOF and fgets() a NULL). try_fgetc() calls clearerr()
 * on "file" before fgetc(), because some Unixes fgetc() remember old EOFs and
 * only return those until explicitely cleared.
 */
extern int try_fgetc(FILE * file, const char * f);
extern char * try_fgets(char * line, int size, FILE * file, const char * f);

/* Return "path" + suffix "_tmp". Value is malloc'd, must be freed externally.*/
extern char * build_temp_path(char * path);

/* Write to "path_tmp" "path" + "_tmp" and return a new file at that "path_tmp"
 * open for writing. "path_tmp" is malloc()'d, must be freed externally.
*/
extern FILE * atomic_write_start(char * path, char ** path_tmp);

/* Finish atomic writing started in atomic_write_start(). Wraps successive calls
 * of fclose() on "file", then unlink() on file at path "path" if it exists,
 * then rename() from "path_tmp" to "path", then free() on "path_tmp".
 */
extern void atomic_write_finish(FILE * file, char * path, char * path_tmp);

/* Check for temp file leftover of atomic writing of "path", abort if found. */
extern void detect_atomic_leftover(char * path);

/* Return largest line length from "file" (including  newline chars). */
extern uint32_t textfile_width(FILE * file);

/* Read "file" for load of non-zero bytes to put onto "queue" string. */
extern uint8_t read_file_into_queue(FILE * file, char ** queue);

/* Return message from "queue" (identified as \n-delimited string, with \n as
 * \0 in the returned message; return nothing if no \n delimiter found).
 */
extern char * get_message_from_queue(char ** queue);



#endif
