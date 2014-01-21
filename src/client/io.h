/* src/client/io.h
 *
 * Communication of the client with the server (by reading and writing files)
 * and the user (by writing to the screen and reading keypresses).
 */

#ifndef IO_H
#define IO_H



/* Try sending "msg" to the server by writing it to the file at
 * world.path_server_in. Try to open it 2^16 times before giving up. After
 * opening, try to write to it 2^16 times before giving up.
 */
extern void try_send(char * msg);

/* Keep checking for user input and a changed server out file. Update client's
 * world representation on out file changes. Manipulate the client and send
 * commands to server based on the user input as interpreted by the control.h
 * library. On each change / activity, re-draw the windows with draw_all_wins().
 * When the loop ends regularly (due to the user sending a quit command), return
 * an appropriate quit message to write to stdout when the client winds down.
 */
extern char * io_loop();



#endif
