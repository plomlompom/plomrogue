/* map_object_actions.h
 *
 * Routines for the actions available to map objects.
 */

#ifndef MAP_OBJECT_ACTIONS_H
#define MAP_OBJECT_ACTIONS_H

#include "yx_uint16.h" /* for yx_uint16 coordinates */
struct Map;
struct MapObj;



/* Try to move "actor" one step in direction "d" (where east is 'E', north 'N'
 * etc.) and handle the consequences: either the move succeeds, or another actor
 * is encountered and hit (which leads to its lifepoint decreasing by one and
 * potentially its death), or the target square is not passable, the move fails.
 */
extern uint8_t move_actor(struct MapObj * actor, char d);

/* Wrapper for using move_actor() on the MapObj representing the player; updates
 * the game log with appropriate messages on the move attempt and its results;
 * turns over to turn_over() when finished.
 */
extern void move_player(char d);

/* Make player wait one turn, i.e. only update_log with a "you wait" message
 * and turn control over to the enemy.
 */
extern void player_wait();

/* Check if coordinate pos on (or beyond) map is accessible to map object
 * movement.
 */
extern char is_passable(struct Map * map, struct yx_uint16 pos);

/* Make player drop to ground map ojbect indexed by world.inventory_select. */
extern void player_drop();

/* Make player pick up map object from ground. */
extern void player_pick();

/* Make player use object indexed by world.inventory_select. */
extern void player_use();



#endif
