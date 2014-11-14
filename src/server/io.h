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
 * (Queueing ensures only complete messages are interpreted, and io_loop()'s
 * server tests only happening in a multi-second interval.)
 */
extern char * io_round();

/* Write to savefile (atomically) god commands (one per line) to rebuild the
 * current world state.
 */
extern void save_world();



#endif
