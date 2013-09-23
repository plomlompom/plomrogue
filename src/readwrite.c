/* readwrite.c */

#include "readwrite.h"
#include <stdio.h>  /* for FILE typedef, fopen(), fgetc(), fputc(), fseek(),
                     * sprintf(), fwrite(), ferror()
                     */
#include <stdint.h> /* for uint8_t, uint16_t, uint32_t */
#include <string.h> /* for strlen()*/
#include <unistd.h> /* for unlink() */
#include "rexit.h"  /* for exit_err() */
#include "misc.h"   /* for trouble_msg() */
struct World;



/* Read/write "x" from/to "file" as bigendian representation of "size" bits. On
 * failure, return 1, else 0. (As of of now, all extern read/write functions
 * build on top of these.)
 *
 * Only use multiples of 8 greater or equal 32 for "size", so that storage
 * inside uint32_t is possible. Originally a bit number check prefaced the code
 * of both functions. It was removed as redundant due to all possible "size"
 * values being hardcoded into the library (i.e. in all extern functions calling
 * / wrapping around either function). If this ever changes, (re-)insert:
 *
 *    if (0 == size || size > 32 || 0 != size % 8)
 *    {
 *        return 1;
 *    }
 */
static uint8_t read_uintX_bigendian(FILE * file, uint32_t * x, uint8_t size);
static uint8_t write_uintX_bigendian(FILE * file, uint32_t x, uint8_t size);



extern FILE * try_fopen(char * path, char * mode, struct World * w, char * f)
{
    char * msg1 = "Trouble in ";
    char * msg2 = " with fopen() (mode '";
    char * msg3 = "') on path '";
    char * msg4 = "'.";
    uint16_t size = strlen(msg1) + strlen(msg2) + strlen(msg3) + strlen(msg4)
                    + strlen(f) + strlen(path) + strlen(mode) + 1;
    char msg[size];
    sprintf(msg, "%s%s%s%s%s%s%s", msg1, f, msg2, mode, msg3, path, msg4);
    FILE * file_p = fopen(path, mode);
    exit_err(NULL == file_p, w, msg);
    return file_p;
}



extern void try_fclose(FILE * file, struct World * w, char * f)
{
    char * msg = trouble_msg(w, f, "fclose()");
    exit_err(fclose(file), w, msg);
    free(msg);
}



extern char * try_fgets(char * line, int linemax, FILE * file,
                        struct World * w, char * f)
{
    char * msg = trouble_msg(w, f, "fgets()");
    char * test = fgets(line, linemax, file);
    exit_err(NULL == test && ferror(file), w, msg);
    free(msg);
    return test;
}



extern void try_fwrite(void * ptr, size_t size, size_t nmemb, FILE * stream,
                       struct World * w, char * f)
{
    char * msg = trouble_msg(w, f, "fwrite()");
    exit_err(0 == fwrite(ptr, size, nmemb, stream), w, msg);
    free(msg);
}



extern void try_fclose_unlink_rename(FILE * file, char * p1, char * p2,
                                     struct World * w, char * f)
{
    try_fclose(file, w, f);
    char * msg1 = "Trouble in ";
    char * msg4 = "'.";
    if (!access(p2, F_OK))
    {
        char * msg2 = " with unlink() on path '";
        uint16_t size = strlen(msg1) + strlen(msg2) + strlen(msg4)
                        + strlen(f) + strlen(p2) + 1;
        char msg[size];
        sprintf(msg, "%s%s%s%s%s", msg1, f, msg2, p2, msg4);
        exit_err(unlink(p2), w, msg);
    }
    char * msg2 = " with rename() from '";
    char * msg3 = "' to '";
    uint16_t size = strlen(msg1) + strlen(f) + strlen(msg2) + strlen(p1)
                    + strlen(msg3) + strlen(p2) + strlen(msg4) + 1;
    char msg[size];
    sprintf(msg, "%s%s%s%s%s%s%s", msg1, f, msg2, p1, msg3, p2, msg4);
    exit_err(rename(p1, p2), w, msg);
}



extern uint16_t get_linemax(FILE * file, struct World * w, char * f)
{
    char * msg = trouble_msg(w, f, "textfile_sizes()");
    uint16_t linemax;
    exit_err(textfile_sizes(file, &linemax, NULL), w, msg);
    free(msg);
    return linemax;
}



extern uint8_t textfile_sizes(FILE * file, uint16_t * linemax_p,
                              uint16_t * n_lines_p)
{
    int c = 0;
    uint16_t c_count = 0;
    uint16_t n_lines = 0;
    uint16_t linemax = 0;
    while (1)
    {
        c = fgetc(file);
        if (EOF == c)
        {
            break;
        }
        c_count++;
        if ('\n' == c)
        {
            if (c_count > linemax)
            {
                linemax = c_count;
            }
            c_count = 0;
            if (n_lines_p)
            {
                n_lines++;
            }
        }
    }
    if (0 == linemax && 0 < c_count) /* Handle files that consist of only one */
    {                                /* line / lack newline chars.            */
        linemax = c_count;
    }

    if (-1 == fseek(file, 0, SEEK_SET))
    {
        return 1;
    }
    * linemax_p = linemax;
    if (n_lines_p)
    {
        * n_lines_p = n_lines;
    }
    return 0;
}



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
