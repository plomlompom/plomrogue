/* src/server/thing_actions.h
 *
 * Actions that can be performed by living things / "actors". Note that apart
 * from the consequences described below, each action may also trigger log
 * messages and other minor stuff if the actor is equal to the player.
 */

#ifndef THING_ACTIONS_H
#define THING_ACTIONS_H

#include <stdint.h> /* uint8_t */
struct Thing;



struct ThingAction
{
    uint8_t id; /* identifies action in Thing.command; therefore must be >0 */
    struct ThingAction * next;
    void (* func) (struct Thing *); /* function called after .effort turns */
    char * name; /* human-readable identifier */
    uint8_t effort; /* how many turns the action takes */
};



/* Free ThingAction * chain starting at "ta". */
extern void free_thing_actions(struct ThingAction * ta);

/* Return world.thing_actions ThingAction.id for "name". */
extern uint8_t get_thing_action_id_by_name(char * name);

/* Actor "t" does nothing. */
extern void actor_wait(struct Thing * t);

/* Actor "t" tries to move one step in direction described by char t->arg (where
 * north-east is 'e', east 'd' etc.) Move either succeeds, or another actor is
 * encountered and hit (which leads ot its lifepoint decreasing by one and
 * eventually death), or the move fails due to an impassable target square. On
 * success, update thing's field of view map.
 */
extern void actor_move(struct Thing * t);

/* Actor "t" tries to drop from inventory thing indexed by number t->args. */
extern void actor_drop(struct Thing * t);

/* Actor "t" tries to pick up topmost thing from ground into its inventory. */
extern void actor_pick(struct Thing * t);

/* Actor "t" tries to use thing in inventory indexed by number t->args.
 * (Currently the only valid use is consuming items defined as consumable.)
 */
extern void actor_use(struct Thing * t);



#endif
