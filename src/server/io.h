/* io.h:
 *
 * Communication of the server with the outside world and its client via input,
 * output and world state files.
 */

#ifndef IO_H
#define IO_H



/* Return single \0-terminated string read from input queue (world.queue); or,
 * if queue is empty and world.turn is unequal world.last_update_turn, update
 * world state file at world.path_worldstate (and update world.last_update_turn
 * and write a single dot line to output file at world.path_out), then read file
 * at world.path_in for the next load of bytes to put onto the input queue.
 *
 * Reading the file at world.path_in may put many \0-terminated strings on the
 * queue at once. Successive calls of io_round() will make these available one
 * by one. Each such call cuts off bytes from the beginning of world.queue, up
 * to and including the last \0 byte that is followed by a non-\0 byte or ends
 * the queue. If the queue starts with a \0 byte, it and its \0 followers are
 * cut and a NULL pointer is returned. Reading from the input file stops only
 * when one or more byte were read and the next read returns 0 bytes. If the
 * re-filled queue does not end in a \0 byte, a \0 byte is appended to it.
 */
extern char * io_round();



#endif
