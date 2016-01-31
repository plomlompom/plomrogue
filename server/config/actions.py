from server.actions import actor_wait, actor_move, actor_pick_up, actor_drop, \
    actor_use

action_db = {
    "actor_wait": actor_wait,
    "actor_move": actor_move,
    "actor_pick_up": actor_pick_up,
    "actor_drop": actor_drop,
    "actor_use": actor_use
}
