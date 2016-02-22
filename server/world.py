# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db
from server.io import log
from server.utils import rand, libpr
from server.utils import id_setter


def update_map_memory(t, age_map=True):
    """Update t's T_MEMMAP with what's in its FOV now,age its T_MEMMEPTHMAP."""
    from server.utils import c_pointer_to_bytearray

    def age_some_memdepthmap_on_nonfov_cells():
        # OUTSOURCED FOR PERFORMANCE REASONS TO libplomrogue.so:
        # ord_v = ord("v")
        # ord_0 = ord("0")
        # ord_9 = ord("9")
        # for pos in [pos for pos in range(world_db["MAP_LENGTH"] ** 2)
        #             if not ord_v == t["fovmap"][pos]
        #             if ord_0 <= t["T_MEMDEPTHMAP"][pos]
        #             if ord_9 > t["T_MEMDEPTHMAP"][pos]
        #             if not rand.next() % (2 **
        #                                   (t["T_MEMDEPTHMAP"][pos] - 48))]:
        #     t["T_MEMDEPTHMAP"][pos] += 1
        memdepthmap = c_pointer_to_bytearray(t["T_MEMDEPTHMAP"])
        fovmap = c_pointer_to_bytearray(t["fovmap"])
        libpr.age_some_memdepthmap_on_nonfov_cells(memdepthmap, fovmap)

    if not t["T_MEMMAP"]:
        t["T_MEMMAP"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
    if not t["T_MEMDEPTHMAP"]:
        t["T_MEMDEPTHMAP"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
    ord_v = ord("v")
    ord_0 = ord("0")
    for pos in [pos for pos in range(world_db["MAP_LENGTH"] ** 2)
                if ord_v == t["fovmap"][pos]]:
        t["T_MEMDEPTHMAP"][pos] = ord_0
        t["T_MEMMAP"][pos] = world_db["MAP"][pos]
    if age_map:
        age_some_memdepthmap_on_nonfov_cells()
    t["T_MEMTHING"] = [mt for mt in t["T_MEMTHING"]
                       if ord_v != t["fovmap"][(mt[1] * world_db["MAP_LENGTH"])
                                               + mt[2]]]
    for id in [id for id in world_db["Things"]
               if not world_db["Things"][id]["carried"]]:
        type = world_db["Things"][id]["T_TYPE"]
        if not world_db["ThingTypes"][type]["TT_LIFEPOINTS"]:
            y = world_db["Things"][id]["T_POSY"]
            x = world_db["Things"][id]["T_POSX"]
            if ord_v == t["fovmap"][(y * world_db["MAP_LENGTH"]) + x]:
                t["T_MEMTHING"].append((type, y, x))


def decrement_lifepoints(t):
    """Decrement t's lifepoints by 1, and if to zero, corpse it.

    If t is the player avatar, only blank its fovmap, so that the client may
    still display memory data. On non-player things, erase fovmap and memory.
    Dying actors drop all their things.
    """
    t["T_LIFEPOINTS"] -= 1
    if 0 == t["T_LIFEPOINTS"]:
        for id in t["T_CARRIES"]:
            t["T_CARRIES"].remove(id)
            world_db["Things"][id]["T_POSY"] = t["T_POSY"]
            world_db["Things"][id]["T_POSX"] = t["T_POSX"]
            world_db["Things"][id]["carried"] = False
        t["T_TYPE"] = world_db["ThingTypes"][t["T_TYPE"]]["TT_CORPSE_ID"]
        if world_db["Things"][0] == t:
            t["fovmap"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
            log("You die.")
            log("See README on how to start over.")
        else:
            t["fovmap"] = False
            t["T_MEMMAP"] = False
            t["T_MEMDEPTHMAP"] = False
            t["T_MEMTHING"] = []


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


def make_world(seed):
    """(Re-)build game world, i.e. map, things, to a new turn 1 from seed.

    Seed rand with seed. Do more only with a "wait" ThingAction and
    world["PLAYER_TYPE"] matching ThingType of TT_START_NUMBER > 0. Then,
    world_db["Things"] emptied, call make_map() and set
    world_db["WORLD_ACTIVE"], world_db["TURN"] to 1. Build new Things
    according to ThingTypes' TT_START_NUMBERS, with Thing of ID 0 to ThingType
    of ID = world["PLAYER_TYPE"]. Place Things randomly, and actors not on each
    other. Init player's memory map. Write "NEW_WORLD" line to out file.
    """
    from server.config.world_data import symbols_passable
    from server.config.misc import make_map_func

    def free_pos():
        i = 0
        while 1:
            err = "Space to put thing on too hard to find. Map too small?"
            while 1:
                y = rand.next() % world_db["MAP_LENGTH"]
                x = rand.next() % world_db["MAP_LENGTH"]
                if chr(world_db["MAP"][y * world_db["MAP_LENGTH"] + x]) in \
                    symbols_passable:
                    break
                i += 1
                if i == 65535:
                    raise SystemExit(err)
            # Replica of C code, wrongly ignores animatedness of new Thing.
            pos_clear = (0 == len([id for id in world_db["Things"]
                                   if world_db["Things"][id]["T_LIFEPOINTS"]
                                   if world_db["Things"][id]["T_POSY"] == y
                                   if world_db["Things"][id]["T_POSX"] == x]))
            if pos_clear:
                break
        return (y, x)

    rand.seed = seed 
    if world_db["MAP_LENGTH"] < 1:
        print("Ignoring: No map length >= 1 defined.")
        return
    libpr.set_maplength(world_db["MAP_LENGTH"])
    player_will_be_generated = False
    playertype = world_db["PLAYER_TYPE"]
    for ThingType in world_db["ThingTypes"]:
        if playertype == ThingType:
            if 0 < world_db["ThingTypes"][ThingType]["TT_START_NUMBER"]:
                player_will_be_generated = True
            break
    if not player_will_be_generated:
        print("Ignoring: No player type with start number >0 defined.")
        return
    wait_action = False
    for ThingAction in world_db["ThingActions"]:
        if "wait" == world_db["ThingActions"][ThingAction]["TA_NAME"]:
            wait_action = True
    if not wait_action:
        print("Ignoring beyond SEED_MAP: " +
              "No thing action with name 'wait' defined.")
        return
    world_db["Things"] = {}
    make_map_func()
    world_db["WORLD_ACTIVE"] = 1
    world_db["TURN"] = 1
    from server.new_thing import new_Thing
    for i in range(world_db["ThingTypes"][playertype]["TT_START_NUMBER"]):
        id = id_setter(-1, "Things")
        world_db["Things"][id] = new_Thing(playertype, free_pos())
    if not world_db["Things"][0]["fovmap"]:
        empty_fovmap = bytearray(b" " * world_db["MAP_LENGTH"] ** 2)
        world_db["Things"][0]["fovmap"] = empty_fovmap
    update_map_memory(world_db["Things"][0])
    for type in world_db["ThingTypes"]:
        for i in range(world_db["ThingTypes"][type]["TT_START_NUMBER"]):
            if type != playertype:
                id = id_setter(-1, "Things")
                world_db["Things"][id] = new_Thing(type, free_pos())
    from server.config.io import io_db
    from server.io import strong_write
    strong_write(io_db["file_out"], "NEW_WORLD\n")


def turn_over():
    """Run game world and its inhabitants until new player input expected."""
    from server.config.actions import action_db, ai_func
    from server.config.misc import thingproliferation_func
    id = 0
    whilebreaker = False
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
                        whilebreaker = True
                        break
                    ai_func(Thing)
                try_healing(Thing)
                hunger(Thing)
                if Thing["T_LIFEPOINTS"]:
                    Thing["T_PROGRESS"] += 1
                    taid = [a for a in world_db["ThingActions"]
                              if a == Thing["T_COMMAND"]][0]
                    ThingAction = world_db["ThingActions"][taid]
                    if Thing["T_PROGRESS"] == ThingAction["TA_EFFORT"]:
                        action = action_db["actor_" + ThingAction["TA_NAME"]]
                        action(Thing)
                        Thing["T_COMMAND"] = 0
                        Thing["T_PROGRESS"] = 0
            thingproliferation_func(Thing, proliferable_map)
        if whilebreaker:
            break
        world_db["TURN"] += 1
