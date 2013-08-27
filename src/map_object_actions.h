/* map_object_actions.h
 *
 * Routines for the actions available to map objects.
 */

#ifndef MAP_OBJECT_ACTIONS_H
#define MAP_OBJECT_ACTIONS_H



#include "yx_uint16.h" /* for yx_uint16 coordinates */
struct World;
struct Map;
struct Monster;



/* Try to move "monster" in random direction. On contact with other monster,
 * only bump. On contact with player, fight / reduce player's hitpoints,
 * and thereby potentially trigger the player's death. Update the log for any
 * contact action.
 */
extern void move_monster(struct World * world, struct Monster * monster);



/* Try to move player in direction "d". On contact with monster, fight / reduce
 * monster's hitpoints, and thereby potentially trigger the monster's death,
 * create a corpse and increment the player's score by the amount of hitpoints
 * the monster started with. Update the log on whatever the player did and turn
 * control over to the enemy.
 */
extern void move_player (struct World * world, enum dir d);



/* Make player wait one turn, i.e. only update_log with a "you wait" message
 * and turn control over to the enemy.
 */
extern void player_wait(struct World * world);



/* Check if coordinate pos on (or beyond) map is accessible to map object
 * movement.
 */
extern char is_passable (struct Map * map, struct yx_uint16 pos);



#endif
