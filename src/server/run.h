/* src/server/run.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Process commands and act on them. Stuff that furthers the state of the game.
 */

#ifndef RUN_H
#define RUN_H

#include <stdint.h> /* uint8_t */



/* Append "answer" to server output file, with instant fflush() if "flush". */
extern void send_to_outfile(char * answer, uint8_t flush);

/* Record save and record file data. Both are only written if "force" is set, or
 * on the first run with unset "force", or if 15 seconds have passed since the
 * last file writing. "msg" is appended to the record file if it is set.
 */
extern void record(char * msg, uint8_t force);

/* Try parsing "msg" into a command to apply, and apply it. Output commands to
 * stdout if "do_verbose" and world.is_verbose are set. If "do_record" is set,
 * record commands to record file, and run save_world() if the last call to it
 * via this function has not happened yet or is at least one minute in the past.
 */
extern void obey_msg(char * msg, uint8_t do_record, uint8_t do_verbose);

/* Loop for receiving commands via io_round(), and acting on them. Exits with 1
 * on "QUIT" command. In replay mode, exits with 0 on each non-"QUIT" command.
 * In play mode, processes incomming god and player commands via obey_msg().
 * Compares the first line of the server out file with world.server_test to
 * ensure that the current server process has not been superseded by a new one.
 */
extern uint8_t io_loop();



#endif
