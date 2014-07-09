/* src/common/readwrite.c */

#include "readwrite.h"
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT32_MAX */
#include <stdio.h> /* FILE, fseek(), sprintf(), fgets(), fgetc(), ferror(),
                    * fputc(), fwrite(), fclose(), fopen(), clearerr()
                    */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen() */
#include <unistd.h> /* access(), unlink() */
#include "rexit.h" /* exit_err(), exit_trouble() */
#include "try_malloc.h" /* try_malloc() */



extern FILE * try_fopen(char * path, char * mode, char * f)
{
    char * f_name = "try_fopen()";
    char * msg1 = "Trouble in ";
    char * msg2 = " with fopen() (mode '";
    char * msg3 = "') on path '";
    char * msg4 = "'.";
    uint16_t size = strlen(msg1) + strlen(msg2) + strlen(msg3) + strlen(msg4)
                    + strlen(f) + strlen(path) + strlen(mode) + 1;
    char * msg = try_malloc(size, f_name);
    sprintf(msg, "%s%s%s%s%s%s%s", msg1, f, msg2, mode, msg3, path, msg4);
    FILE * file_p = fopen(path, mode);
    exit_err(NULL == file_p, msg);
    free(msg);
    return file_p;
}



extern void try_fclose(FILE * file, char * f)
{
    exit_trouble(fclose(file), f, "fclose()");
}



extern void try_fwrite(void * ptr, size_t size, size_t nmemb, FILE * stream,
                       char * f)
{
    exit_trouble(0 == fwrite(ptr, size, nmemb, stream), f, "fwrite()");
}



extern void try_fputc(uint8_t c, FILE * file, char * f)
{
    exit_trouble(EOF == fputc(c, file), f, "fputc()");
}



extern int try_fgetc(FILE * file, char * f)
{
    clearerr(file); /* OSX' (BSD?) fgetc() needs this to undo previous EOFs. */
    int test = fgetc(file);
    exit_trouble(EOF == test && ferror(file), f, "fgetc()");
    return test;
}



extern char * try_fgets(char * line, int linemax, FILE * file, char * f)
{
    char * test = fgets(line, linemax, file);
    exit_trouble(NULL == test && ferror(file), f, "fgets()");
    return test;
}



extern void try_fclose_unlink_rename(FILE * file, char * p1, char * p2,
                                     char * f)
{
    char * f_name = "try_fclose_unlink_rename()";
    try_fclose(file, f);
    char * msg1 = "Trouble in ";
    char * msg4 = "'.";
    if (!access(p2, F_OK))
    {
        char * msg2 = " with unlink() on path '";
        uint16_t size = strlen(msg1) + strlen(msg2) + strlen(msg4)
                        + strlen(f) + strlen(p2) + 1;
        char * msg = try_malloc(size, f_name);
        sprintf(msg, "%s%s%s%s%s", msg1, f, msg2, p2, msg4);
        exit_err(unlink(p2), msg);
        free(msg);
    }
    char * msg2 = " with rename() from '";
    char * msg3 = "' to '";
    uint16_t size = strlen(msg1) + strlen(f) + strlen(msg2) + strlen(p1)
                    + strlen(msg3) + strlen(p2) + strlen(msg4) + 1;
    char * msg = try_malloc(size, f_name);
    sprintf(msg, "%s%s%s%s%s%s%s", msg1, f, msg2, p1, msg3, p2, msg4);
    exit_err(rename(p1, p2), msg);
    free(msg);
}



extern uint32_t textfile_width(FILE * file)
{
    char * f_name = "textfile_width()";
    int c = 0;
    uint32_t c_count = 0;
    uint32_t linemax = 0;
    while (1)
    {
        c = try_fgetc(file, f_name);
        if (EOF == c)
        {
            break;
        }
        c_count++;
        exit_trouble(UINT32_MAX == c_count, f_name, "too large text file line");
        if ('\n' == c)
        {
            if (c_count > linemax)
            {
                linemax = c_count;
            }
            c_count = 0;
        }
    }
  if (0 == linemax && 0 < c_count) /* Handle files that consist of only one */
  {                                /* line / lack newline chars.            */
      linemax = c_count;
   }
   exit_trouble(-1 == fseek(file, 0, SEEK_SET), f_name, "fseek()");
   return linemax;
}
