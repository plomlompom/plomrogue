/* src/client/io.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Communication of the client with the server (by reading and writing files)
 * and the user (by writing to the screen and reading keypresses).
 */

#ifndef IO_H
#define IO_H



/* Write "msg" plus newline to server input file at world.path_server_in.
 *
 * "msg"  must fit into size defined by PIPE_BUF so that no race conditiosn
 * arise by many clients writing to the file in parallel.
 */
extern void send(char * msg);

/* Keep checking for user input, a changed worldstate file, and queue input from
 * the server's out file. Update client's world representation on worldstate
 * file changes. Manipulate the client and send commands to server based on the
 * user input as interpreted by the control.h library.
 *
 * On each change / activity, re-draw the windows with draw_all_wins(). When the
 * loop ends regularly (due to the user sending a quit command), return an
 * appropriate quit message to write to stdout when the client winds down. Call
 * reset_windows() on receiving a SIGWINCH. Abort on assumed server death if the
 * server's out file does not get updated, even on PING requests. Re-focus map
 * view on player if world.autofocus is set. Messages from the out file are put
 * together on the queue first, from which only complete (\n-delimited) messages
 * are read. Queues of messages are worked through completely / emptied before
 * any re-drawing or further server polling happens.
 */
extern char * io_loop();



#endif
