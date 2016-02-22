from server.config.world_data import world_db
from server.io import log
from server.utils import rand, libpr, c_pointer_to_bytearray
from server.utils import id_setter


def thingproliferation(t, prol_map):
    """To chance of 1/TT_PROLIFERATE, create t offspring in open neighbor cell.

    Naturally only works with TT_PROLIFERATE > 0. The neighbor cell must be be
    marked "." in prol_map. If there are several map cell candidates, one is
    selected randomly.
    """
    from server.config.world_data import directions_db, symbols_passable
    from server.utils import mv_yx_in_dir_legal
    prolscore = world_db["ThingTypes"][t["T_TYPE"]]["TT_PROLIFERATE"]
    if prolscore and (1 == prolscore or 1 == (rand.next() % prolscore)):
        candidates = []
        for dir in [directions_db[key] for key in sorted(directions_db.keys())]:
            mv_result = mv_yx_in_dir_legal(dir, t["T_POSY"], t["T_POSX"])
            c = prol_map[mv_result[1] + world_db["MAP_LENGTH"] + mv_result[2]]
            if mv_result[0] and str(c) in symbols_passable:
                candidates.append((mv_result[1], mv_result[2]))
        if len(candidates):
            i = rand.next() % len(candidates)
            id = id_setter(-1, "Things")
            newT = new_Thing(t["T_TYPE"], (candidates[i][0], candidates[i][1]))
            world_db["Things"][id] = newT


def update_map_memory(t, age_map=True):
    """Update t's T_MEMMAP with what's in its FOV now,age its T_MEMMEPTHMAP."""

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


def build_fov_map(t):
    """Build Thing's FOV map."""
    t["fovmap"] = bytearray(b'v' * (world_db["MAP_LENGTH"] ** 2))
    fovmap = c_pointer_to_bytearray(t["fovmap"])
    map = c_pointer_to_bytearray(world_db["MAP"])
    if libpr.build_fov_map(t["T_POSY"], t["T_POSX"], fovmap, map):
        raise RuntimeError("Malloc error in build_fov_Map().")


def new_Thing(_type, pos=(0, 0)):
    """Return Thing of type T_TYPE, with fovmap if alive and world active."""
    from server.config.world_data import thing_defaults
    thing = {}
    for key in thing_defaults:
        thing[key] = thing_defaults[key]
        if type(thing[key]) == list:
            thing[key] = thing[key][:]
    thing["T_LIFEPOINTS"] = world_db["ThingTypes"][_type]["TT_LIFEPOINTS"]
    thing["T_TYPE"] = _type
    thing["T_POSY"] = pos[0]
    thing["T_POSX"] = pos[1]
    if world_db["WORLD_ACTIVE"] and thing["T_LIFEPOINTS"]:
        build_fov_map(thing)
    return thing


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


def make_map():
    """(Re-)make island map.

    Let "~" represent water, "." land, "X" trees: Build island shape randomly,
    start with one land cell in the middle, then go into cycle of repeatedly
    selecting a random sea cell and transforming it into land if it is neighbor
    to land. The cycle ends when a land cell is due to be created at the map's
    border. Then put some trees on the map (TODO: more precise algorithm desc).
    """

    def is_neighbor(coordinates, type):
        y = coordinates[0]
        x = coordinates[1]
        length = world_db["MAP_LENGTH"]
        ind = y % 2
        diag_west = x + (ind > 0)
        diag_east = x + (ind < (length - 1))
        pos = (y * length) + x
        if (y > 0 and diag_east
            and type == chr(world_db["MAP"][pos - length + ind])) \
           or (x < (length - 1)
               and type == chr(world_db["MAP"][pos + 1])) \
           or (y < (length - 1) and diag_east
               and type == chr(world_db["MAP"][pos + length + ind])) \
           or (y > 0 and diag_west
               and type == chr(world_db["MAP"][pos - length - (not ind)])) \
           or (x > 0
               and type == chr(world_db["MAP"][pos - 1])) \
           or (y < (length - 1) and diag_west
               and type == chr(world_db["MAP"][pos + length - (not ind)])):
            return True
        return False

    world_db["MAP"] = bytearray(b'~' * (world_db["MAP_LENGTH"] ** 2))
    length = world_db["MAP_LENGTH"]
    add_half_width = (not (length % 2)) * int(length / 2)
    world_db["MAP"][int((length ** 2) / 2) + add_half_width] = ord(".")
    while (1):
        y = rand.next() % length
        x = rand.next() % length
        pos = (y * length) + x
        if "~" == chr(world_db["MAP"][pos]) and is_neighbor((y, x), "."):
            if y == 0 or y == (length - 1) or x == 0 or x == (length - 1):
                break
            world_db["MAP"][pos] = ord(".")
    n_trees = int((length ** 2) / 16)
    i_trees = 0
    while (i_trees <= n_trees):
        single_allowed = rand.next() % 32
        y = rand.next() % length
        x = rand.next() % length
        pos = (y * length) + x
        if "." == chr(world_db["MAP"][pos]) \
                and ((not single_allowed) or is_neighbor((y, x), "X")):
            world_db["MAP"][pos] = ord("X")
            i_trees += 1
    # This all-too-precise replica of the original C code misses iter_limit().


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
    make_map()
    world_db["WORLD_ACTIVE"] = 1
    world_db["TURN"] = 1
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
            thingproliferation(Thing, proliferable_map)
        if whilebreaker:
            break
        world_db["TURN"] += 1
