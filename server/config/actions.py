# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.actions import actor_wait, actor_move, actor_pickup, actor_drop, \
    actor_use


action_db = {
    "actor_wait": actor_wait,
    "actor_move": actor_move,
    "actor_pickup": actor_pickup,
    "actor_drop": actor_drop,
    "actor_use": actor_use
}
actor_pickup_test_hook = lambda x, y: True
actor_use_attempts_hook = lambda x, y: None
