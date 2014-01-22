/* src/server/run.h
 *
 * Process commands and act on them. Stuff that furthers the state of the game.
 */

#ifndef RUN_H
#define RUN_H

#include <stdint.h> /* uint8_t */



/* Try parsing "msg" into a server or player command to run. Player commands are
 * are recorded into the record file at world.path_record if "do_record" is set.
 */
extern void obey_msg(char * msg, uint8_t do_record);

/* Loop for receiving commands via io_round() and acting on them. Exits with 1
 * on "QUIT" command. In replay mode, exits with 0 on each non-"QUIT" command.
 * In play mode, processes incomming commands via obey_msg().
 */
extern uint8_t io_loop();



#endif