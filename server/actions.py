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
    """If passable, move/collide(=attack) thing into T_ARGUMENT's direction."""
    from server.world import build_fov_map, decrement_lifepoints
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
            if t == world_db["Things"][0]:
                hitted_type = world_db["Things"][hit_id]["T_TYPE"]
                hitted_name = world_db["ThingTypes"][hitted_type]["TT_NAME"]
                log("You WOUND " + hitted_name + ".")
            elif 0 == hit_id:
                hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
                log(hitter_name +" WOUNDS you.")
            decrement_lifepoints(world_db["Things"][hit_id])
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


def actor_pickup(t):
    """Make t pick up (topmost?) Thing from ground into inventory.

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
            if lowest_tid == -1 or tid < lowest_tid:
                id = iid
                lowest_tid = tid
        world_db["Things"][id]["carried"] = True
        t["T_CARRIES"].append(id)
        if t == world_db["Things"][0]:
                log("You PICK UP an object.")


def actor_drop(t):
    """Make t rop Thing from inventory to ground indexed by T_ARGUMENT."""
    # TODO: Handle case where T_ARGUMENT matches nothing.
    if len(t["T_CARRIES"]):
        id = t["T_CARRIES"][t["T_ARGUMENT"]]
        t["T_CARRIES"].remove(id)
        world_db["Things"][id]["carried"] = False
        if t == world_db["Things"][0]:
            log("You DROP an object.")


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
                log("You CONSUME this object.")
        elif t == world_db["Things"][0]:
            log("You try to use this object, but FAIL.")
