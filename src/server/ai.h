/* src/server/ai.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Pseudo AI for actor movement.
 */

#ifndef AI_H
#define AI_H

struct Thing;



/* Determine next non-player actor command / arguments by the actor's AI. It's
 * pretty dumb so far. Actors will try to move towards their path-wise nearest
 * neighbor. If no one else is found in the neighborhood, they will simply wait.
 */
extern void ai(struct Thing * t);



#endif
