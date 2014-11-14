/* src/common/readwrite.c
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 */

#include "readwrite.h"
#include <stddef.h> /* NULL, size_t */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, UINT32_MAX */
#include <stdio.h> /* FILE, fseek(), sprintf(), fgets(), fgetc(), ferror(),
                    * fputc(), fwrite(), fclose(), fopen(), clearerr()
                    */
#include <stdlib.h> /* free() */
#include <string.h> /* strlen(), memcpy() */
#include <unistd.h> /* access(), unlink() */
#include "rexit.h" /* exit_err(), exit_trouble() */
#include "try_malloc.h" /* try_malloc() */



extern FILE * try_fopen(char * path, char * mode, const char * f)
{
    char * msg1 = "Trouble in ";
    char * msg2 = " with fopen (mode '";
    char * msg3 = "') on path '";
    char * msg4 = "'.";
    uint16_t size = strlen(msg1) + strlen(msg2) + strlen(msg3) + strlen(msg4)
                    + strlen(f) + strlen(path) + strlen(mode) + 1;
    char * msg = try_malloc(size, __func__);
    int test = sprintf(msg, "%s%s%s%s%s%s%s", msg1,f,msg2,mode,msg3,path,msg4);
    exit_trouble(test < 0, __func__, "sprintf");
    FILE * file_p = fopen(path, mode);
    exit_err(!file_p, msg);
    free(msg);
    return file_p;
}



extern void try_fclose(FILE * file, const char * f)
{
    exit_trouble(fclose(file), f, "fclose");
}



extern void try_fwrite(void * ptr, size_t size, size_t nmemb, FILE * stream,
                       const char * f)
{
    exit_trouble(0 == fwrite(ptr, size, nmemb, stream), f, "fwrite");
}



extern void try_fputc(uint8_t c, FILE * file, const char * f)
{
    exit_trouble(EOF == fputc(c, file), f, "fputc");
}



extern int try_fgetc(FILE * file, const char * f)
{
    clearerr(file); /* OSX' (BSD?) fgetc() needs this to undo previous EOFs. */
    int test = fgetc(file);
    exit_trouble(EOF == test && ferror(file), f, "fgetc");
    return test;
}



extern char * try_fgets(char * line, int linemax, FILE * file, const char * f)
{
    char * test = fgets(line, linemax, file);
    exit_trouble(!test && ferror(file), f, "fgets");
    return test;
}



extern char * build_temp_path(char * path)
{
    char * suffix_tmp = "_tmp";
    uint16_t size = strlen(path) + strlen(suffix_tmp) + 1;
    char * path_tmp = try_malloc(size, __func__);
    int test = sprintf(path_tmp, "%s%s", path, suffix_tmp);
    exit_trouble(test < 0, __func__, "sprintf");
    return path_tmp;
}



extern FILE * atomic_write_start(char * path, char ** path_tmp)
{
    *path_tmp = build_temp_path(path);
    return try_fopen(*path_tmp, "w", __func__);
}



extern void atomic_write_finish(FILE * file, char * path, char * path_tmp)
{
    try_fclose(file, __func__);
    char * msg1 = "Trouble in ";
    char * msg4 = "'.";
    if (!access(path, F_OK))
    {
        char * msg2 = " with unlink on path '";
        uint16_t size = strlen(msg1) + strlen(msg2) + strlen(msg4)
                        + strlen(__func__) + strlen(path) + 1;
        char * msg = try_malloc(size, __func__);
        int test = sprintf(msg, "%s%s%s%s%s", msg1, __func__, msg2, path, msg4);
        exit_trouble(test < 0, __func__, "sprintf");
        exit_err(unlink(path), msg);
        free(msg);
    }
    char * msg2 = " with rename from '";
    char * msg3 = "' to '";
    uint16_t size =   strlen(msg1) + strlen(__func__) + strlen(msg2) +
                    + strlen(path_tmp) + strlen(msg3) + strlen(path)
                    + strlen(msg4) + 1;
    char * msg = try_malloc(size, __func__);
    int test = sprintf(msg, "%s%s%s%s%s%s%s",
                            msg1, __func__, msg2, path_tmp, msg3, path, msg4);
    exit_trouble(test < 0, __func__, "sprintf");
    exit_err(rename(path_tmp, path), msg);
    free(msg);
    free(path_tmp);
}



extern void detect_atomic_leftover(char * path)
{
    char * path_tmp = build_temp_path(path);
    char * part1 = "Found file '";
    char * part2 = "' that may be a leftover from an aborted previous attempt "
                   "to write '";
    char * part3 = "'. Aborting until the matter is solved by (re-)moving it.";
    uint32_t size =   strlen(part1) + strlen(path_tmp) + strlen(part2)
                    + strlen(path) + strlen(part3) + 1;
    char * msg = try_malloc(size, __func__);
    int test = sprintf(msg, "%s%s%s%s%s", part1, path_tmp, part2, path, part3);
    exit_trouble(test < 0, __func__, "sprintf");
    exit_err(!access(path_tmp, F_OK), msg);
    free(msg);
    free(path_tmp);
}



extern uint32_t textfile_width(FILE * file)
{
    int c = 0;
    uint32_t c_count = 0;
    uint32_t linemax = 0;
    while (1)
    {
        c = try_fgetc(file, __func__);
        if (EOF == c)
        {
            break;
        }
        c_count++;
        exit_trouble(UINT32_MAX==c_count, __func__, "too large text file line");
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
   exit_trouble(-1 == fseek(file, 0, SEEK_SET), __func__, "fseek");
   return linemax;
}



extern uint8_t read_file_into_queue(FILE * file, char ** queue,
                                    uint32_t * queue_size)
{
    int test = try_fgetc(file, __func__);
    if (EOF != test)
    {
        do
        {
            char c = (char) test;
            if ('\n' == c)
            {
                c = '\0';
            }
            char * new_queue = try_malloc(*queue_size + 1, __func__);
            memcpy(new_queue, *queue, *queue_size);
            char * new_pos = new_queue + *queue_size;
            * new_pos = c;
            *queue_size = *queue_size + 1;
            free(*queue);
            *queue = new_queue;
        }
        while (EOF != (test = try_fgetc(file, __func__)));
        if (*queue_size && '\0' != (*queue)[*queue_size - 1])
        {
            char * new_queue = try_malloc(*queue_size + 1, __func__);
            memcpy(new_queue, *queue, *queue_size);
            new_queue[*queue_size] = '\0';
            *queue_size = *queue_size + 1;
            free(*queue);
            *queue = new_queue;
        }
        return 1;
    }
    return 0;
}



extern char * get_message_from_queue(char ** queue, uint32_t * queue_size)
{
    char * message = NULL;
    if (*queue_size)
    {
        size_t cutout_len = strlen(*queue);
        uint8_t is_nullbyte_chunk = !cutout_len;
        if (0 < cutout_len)
        {
            cutout_len++;
            message = try_malloc(cutout_len, __func__);
            memcpy(message, *queue, cutout_len);
        }
        for (;
             cutout_len != *queue_size && '\0' == (*queue)[cutout_len];
             cutout_len++);
        *queue_size = *queue_size - cutout_len;
        if (0 == *queue_size)
        {
            free(*queue);            /* NULL so read_file_into_queue() and    */
            *queue = NULL;           /* cleanup() may free() this every time, */
        }                            /* even when it's un-allocated.          */
        else
        {
            char * new_queue = try_malloc(*queue_size, __func__);
            memcpy(new_queue, &((*queue)[cutout_len]), *queue_size);
            free(*queue);
            *queue = new_queue;
            if (is_nullbyte_chunk)
            {
                return get_message_from_queue(queue, queue_size);
            }
        }
    }
    return message;
}
