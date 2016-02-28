# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db
from server.io import log


def actor_wait(t):
    """Make t do nothing (but loudly, if player avatar)."""
    if t == world_db["Things"][0]:
        log("You wait")


def actor_move(t):
    """If passable, move/collide(=attack) thing into T_ARGUMENT's direction.

    On attack, return 0 on non-kill and TT_LIFEPOINTS of killed type on kill,
    plus type id of attacked Thing. On move, return mv_yx_in_dir_legal result.
    """
    from server.build_fov_map import build_fov_map
    from server.config.misc import decrement_lifepoints
    from server.utils import mv_yx_in_dir_legal
    from server.config.world_data import directions_db, symbols_passable
    passable = False
    move_result = mv_yx_in_dir_legal(chr(t["T_ARGUMENT"]),
                                     t["T_POSY"], t["T_POSX"])
    if 1 == move_result[0]:
        pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
        hitted = [id for id in world_db["Things"]
                  if world_db["Things"][id] != t
                  if world_db["Things"][id]["T_LIFEPOINTS"]
                  if world_db["Things"][id]["T_POSY"] == move_result[1]
                  if world_db["Things"][id]["T_POSX"] == move_result[2]]
        if len(hitted):
            hit_id = hitted[0]
            hitted_tid = world_db["Things"][hit_id]["T_TYPE"]
            if t == world_db["Things"][0]:
                hitted_name = world_db["ThingTypes"][hitted_tid]["TT_NAME"]
                log("You WOUND " + hitted_name + ".")
            elif 0 == hit_id:
                hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
                log(hitter_name +" WOUNDS you.")
            decr_test = decrement_lifepoints(world_db["Things"][hit_id])
            if decr_test > 0 and t == world_db["Things"][0]:
                log(hitted_name + " dies.")
            return decr_test, hitted_tid
        from server.config.actions import actor_move_attempts_hook
        if actor_move_attempts_hook(t, move_result, pos):
            return
        passable = chr(world_db["MAP"][pos]) in symbols_passable
    dir = [dir for dir in directions_db
           if directions_db[dir] == chr(t["T_ARGUMENT"])][0]
    if passable:
        t["T_POSY"] = move_result[1]
        t["T_POSX"] = move_result[2]
        for id in t["T_CARRIES"]:
            world_db["Things"][id]["T_POSY"] = move_result[1]
            world_db["Things"][id]["T_POSX"] = move_result[2]
        build_fov_map(t)
        if t == world_db["Things"][0]:
            log("You MOVE " + dir + ".")
        return move_result


def actor_pickup(t):
    """Make t pick up (topmost?) Thing from ground into inventory. Return it.

    Define topmostness by how low the thing's type ID is.
    """
    ids = [id for id in world_db["Things"] if world_db["Things"][id] != t
           if not world_db["Things"][id]["carried"]
           if world_db["Things"][id]["T_POSY"] == t["T_POSY"]
           if world_db["Things"][id]["T_POSX"] == t["T_POSX"]]
    if len(ids):
        lowest_tid = -1
        for iid in ids:
            tid = world_db["Things"][iid]["T_TYPE"]
            from server.config.actions import actor_pickup_test_hook
            if (lowest_tid == -1 or tid < lowest_tid) and \
                    actor_pickup_test_hook(t, tid):
                id = iid
                lowest_tid = tid
        world_db["Things"][id]["carried"] = True
        t["T_CARRIES"].append(id)
        if t == world_db["Things"][0]:
                log("You PICK UP an object.")
        return world_db["Things"][id]

def actor_drop(t):
    """Drop to ground from t's inventory, return T_ARGUMENT-indexed Thing."""
    # TODO: Handle case where T_ARGUMENT matches nothing.
    if len(t["T_CARRIES"]):
        id = t["T_CARRIES"][t["T_ARGUMENT"]]
        t["T_CARRIES"].remove(id)
        world_db["Things"][id]["carried"] = False
        if t == world_db["Things"][0]:
            log("You DROP an object.")
            return world_db["Things"][id]

def actor_use(t):
    """Make t use (for now: consume) T_ARGUMENT-indexed Thing in inventory."""
    # TODO: Handle case where T_ARGUMENT matches nothing.
    if len(t["T_CARRIES"]):
        id = t["T_CARRIES"][t["T_ARGUMENT"]]
        type = world_db["Things"][id]["T_TYPE"]
        if world_db["ThingTypes"][type]["TT_TOOL"] == "food":
            t["T_CARRIES"].remove(id)
            del world_db["Things"][id]
            t["T_SATIATION"] += world_db["ThingTypes"][type]["TT_TOOLPOWER"]
            if t == world_db["Things"][0]:
                log("You CONSUME this thing.")
        else:
            from server.config.actions import actor_use_attempts_hook
            actor_use_attempts_hook(t, type)
