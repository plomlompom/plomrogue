/* src/server/map_object_actions.h
 *
 * Actions that can be performed my map objects / "actors". Note that apart
 * from the consequences described below, each action may also trigger log
 * messages and other minor stuff if the actor is equal to the player.
 */

#ifndef MAP_OBJECT_ACTIONS_H
#define MAP_OBJECT_ACTIONS_H

#include <stdint.h> /* uint8_t */
struct MapObj;



struct MapObjAct
{
    struct MapObjAct * next;
    void (* func) (struct MapObj *); /* function called after .effort turns */
    char * name;                     /* human-readable identifier */
    uint8_t id;                      /* unique id of map object action */
    uint8_t effort;                  /* how many turns the action takes */
};



/* Init MapObjAct chain at world.map_obj_acts. */
extern void init_map_object_actions();

/* Free MapObjAct * chain starting at "moa". */
extern void free_map_object_actions(struct MapObjAct * moa);

/* Return world.map_obj_acts MapObjAct.id for "name". */
extern uint8_t get_moa_id_by_name(char * name);

/* Actor "mo" does nothing. */
extern void actor_wait(struct MapObj * mo);

/* Actor "mo" tries to move one step in direction described by char mo->arg
 * (where east is 'E', north 'N') etc. Move either succeeds, or another actor is
 * encountered and hit (which leads ot its lifepoint decreasing by one and
 * eventually death), or the move fails due to an impassable target square.
 */
extern void actor_move(struct MapObj * mo);

/* Actor "mo" tries to drop from inventory object indexed by number mo->args. */
extern void actor_drop(struct MapObj * mo);

/* Actor "mo" tries to pick up object from ground into its inventory. */
extern void actor_pick(struct MapObj * mo);

/* Actor "mo" tries to use inventory object indexed by number mo->args.
 * (Currently the only valid use is consuming an item named "MAGIC MEAT",
 * which increments the actors lifepoints.)
 */
extern void actor_use(struct MapObj * mo);



#endif
