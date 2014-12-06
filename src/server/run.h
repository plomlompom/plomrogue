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

/* Try parsing "msg" into a command to apply. Output "msg" if world.is_verbose.
 * If "obey_state" is > 1 and world.replay is set, any non-meta command message
 * is not executed, but merely returns 3. The QUIT meta command (if well-formed)
 * always returns 2. Other meta commands and (with "obey_state" < 2) non-meta
 * commands return 1 if well-formed. Malformed or empty command messages return
 * 0. If "obey_state" is 1, "msg" is recorded via a non-forced record(). If a
 * non-meta command is executed and world.exists, world.do_update is set.
 */
extern uint8_t obey_msg(char * msg, uint8_t obey_state);

/* Loop to read commands via io_round() and call obey_msg() with "obey_state"
 * set on them. If latter returns 2 and world.replay is not set, or returns > 1
 * and world.replay is set, abort loop to return that result. After io_round(),
 * compares 1st line of the server out file is compared with world.server_test
 * to ensure the server process hasn't been superseded by a new one.
 */
extern uint8_t io_loop(uint8_t obey_state);



#endif
