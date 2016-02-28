# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db
from server.io import log
from server.utils import rand


def try_healing(t):
    """If t's HP < max, increment them if well-nourished, maybe waiting."""
    if t["T_LIFEPOINTS"] < \
       world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]:
        wait_id = [id for id in world_db["ThingActions"]
                      if world_db["ThingActions"][id]["TA_NAME"] == "wait"][0]
        wait_divider = 8 if t["T_COMMAND"] == wait_id else 1
        testval = int(abs(t["T_SATIATION"]) / wait_divider)
        if (testval <= 1 or 1 == (rand.next() % testval)):
            t["T_LIFEPOINTS"] += 1
            if t == world_db["Things"][0]:
                log("You HEAL.")


def hunger_per_turn(type_id):
    """The amount of satiation score lost per turn for things of given type."""
    import math
    return int(math.sqrt(world_db["ThingTypes"][type_id]["TT_LIFEPOINTS"]))


def hunger(t):
    """Decrement t's satiation,dependent on it trigger lifepoint dec chance."""
    from server.config.misc import decrement_lifepoints
    if t["T_SATIATION"] > -32768:
        t["T_SATIATION"] -= hunger_per_turn(t["T_TYPE"])
    if 0 != t["T_SATIATION"] and 0 == int(rand.next() / abs(t["T_SATIATION"])):
        if t == world_db["Things"][0]:
            if t["T_SATIATION"] < 0:
                log("You SUFFER from hunger.")
            else:
                log("You SUFFER from over-eating.")
        decrement_lifepoints(t)


def set_world_inactive():
    """Set world_db["WORLD_ACTIVE"] to 0 and remove worldstate file."""
    from server.io import safely_remove_worldstate_file
    safely_remove_worldstate_file()
    world_db["WORLD_ACTIVE"] = 0


def turn_over():
    """Run game world and its inhabitants until new player input expected."""
    from server.ai import ai
    from server.config.actions import action_db
    from server.config.misc import calc_effort_func
    from server.update_map_memory import update_map_memory
    from server.thingproliferation import thingproliferation
    id = 0
    while world_db["Things"][0]["T_LIFEPOINTS"]:
        proliferable_map = world_db["MAP"][:]
        for id in [id for id in world_db["Things"]
                   if not world_db["Things"][id]["carried"]]:
            y = world_db["Things"][id]["T_POSY"]
            x = world_db["Things"][id]["T_POSX"]
            proliferable_map[y * world_db["MAP_LENGTH"] + x] = ord('X')
        for id in [id for id in world_db["Things"]]:  # Only what's from start!
            if not id in world_db["Things"] or \
               world_db["Things"][id]["carried"]:   # May have been consumed or
                continue                            # picked up during turn …
            Thing = world_db["Things"][id]
            if Thing["T_LIFEPOINTS"]:
                if not Thing["T_COMMAND"]:
                    update_map_memory(Thing)
                    if 0 == id:
                        return
                    ai(Thing)
                try_healing(Thing)
                hunger(Thing)
                if Thing["T_LIFEPOINTS"]:
                    Thing["T_PROGRESS"] += 1
                    taid = [a for a in world_db["ThingActions"]
                              if a == Thing["T_COMMAND"]][0]
                    ThingAction = world_db["ThingActions"][taid]
                    effort = calc_effort_func(ThingAction, Thing)
                    if Thing["T_PROGRESS"] == effort:
                        action = action_db["actor_" + ThingAction["TA_NAME"]]
                        action(Thing)
                        Thing["T_COMMAND"] = 0
                        Thing["T_PROGRESS"] = 0
            thingproliferation(Thing, proliferable_map)
        world_db["TURN"] += 1
