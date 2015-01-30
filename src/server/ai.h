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



/* Determine next non-player actor command / arguments by the actor's AI. Actors
 * will look for, and move towards, enemies (animate things not of their own
 * type); if they see none, they will consume consumables in their inventory; if
 * there are none, they will pick up any consumables they stand on; if they
 * stand on none, they will move towards the next consumable they see or
 * remember on the map; if they see or remember none, they'll explore parts of
 * the map unseen since ever or for at least one turn; if there is nothing to
 * explore, they will simply wait.
 */
extern void ai(struct Thing * t);



#endif
