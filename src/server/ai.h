/* src/server/ai.h
 *
 * Pseudo AI for actor movement.
 */

#ifndef AI_H
#define AI_H

struct MapObj;



/* Determine next non-player actor command / arguments by the actor's AI.
 *
 * The AI is pretty dumb so far. Actors basically try to move towards their
 * nearest neighbor in a straight line, easily getting stuck behind obstacles or
 * ending up in endless chase circles with each other.
 */
extern void pretty_dumb_ai(struct MapObj * mo);



#endif
