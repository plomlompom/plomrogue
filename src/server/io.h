/* io.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Communication of the server with the outside world and its client via input,
 * output and world state files.
 */

#ifndef IO_H
#define IO_H



/* Return single \0-terminated string read from input queue (world.queue); or,
 * if queue is empty and world.do_update is set, update world state file (and
 * unset world.do_update) and write a single dot line to server out file, then
 * read server in file for the next load of bytes to put onto the input queue.
 *
 * Reading the server in file may put many \0-terminated strings on the queue at
 * once. Successive calls of io_round() will make these available one by one.
 * Each such call cuts off bytes from the beginning of world.queue, up to and
 * including the last \0 byte that is followed by a non-\0 byte or ends the
 * queue. If the queue starts with a \0 byte, it and its \0 followers are cut
 * before returning anything after them. Reading from the input file stops only
 * at its end or when one or more byte were read and the next read returns 0
 * bytes. If the re-filled queue doesn't end in \0, a \0 byte is appended to it.
 */
extern char * io_round();

/* Write to savefile (atomically) god commands (one per line) to rebuild the
 * current world state.
 */
extern void save_world();



#endif
