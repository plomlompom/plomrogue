# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.io import log, strong_write
from server.config.world_data import world_db, symbols_passable, directions_db
from server.utils import mv_yx_in_dir_legal, rand, id_setter
from server.config.io import io_db
from server.new_thing import new_Thing

def make_world(seed):
    from server.update_map_memory import update_map_memory
    from server.config.misc import make_map_func
    from server.utils import libpr

    def free_pos(plant=False):
        i = 0
        while 1:
            err = "Space to put thing on too hard to find. Map too small?"
            while 1:
                y = rand.next() % world_db["MAP_LENGTH"]
                x = rand.next() % world_db["MAP_LENGTH"]
                pos = y * world_db["MAP_LENGTH"] + x;
                if (not plant
                    and "." == chr(world_db["MAP"][pos])) \
                   or ":" == chr(world_db["MAP"][pos]):
                    break
                i += 1
                if i == 65535:
                    raise SystemExit(err)
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
    for name in world_db["specials"]:
        if world_db[name] not in world_db["ThingTypes"]:
            print("Ignoring: No valid " + name + " set.")
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
                plantness = world_db["ThingTypes"][type]["TT_PROLIFERATE"]
                world_db["Things"][id] = new_Thing(type, free_pos(plantness))
    strong_write(io_db["file_out"], "NEW_WORLD\n")

def thingproliferation(t, prol_map):
    from server.new_thing import new_Thing
    global directions_db, mv_yx_in_dir_legal
    prolscore = world_db["ThingTypes"][t["T_TYPE"]]["TT_PROLIFERATE"]
    if prolscore and \
      (world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"] == 0 or
       t["T_LIFEPOINTS"] >= 0.9 *
                        world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]) \
       and \
      (1 == prolscore or 1 == (rand.next() % prolscore)):
        candidates = []
        for dir in [directions_db[key] for key in directions_db]:
            mv_result = mv_yx_in_dir_legal(dir, t["T_POSY"], t["T_POSX"])
            pos = mv_result[1] * world_db["MAP_LENGTH"] + mv_result[2]
            if mv_result[0] and \
               (ord(":") == prol_map[pos]
                or (world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]
                    and ord(".") == prol_map[pos])):
                candidates.append((mv_result[1], mv_result[2]))
        if len(candidates):
            i = rand.next() % len(candidates)
            id = id_setter(-1, "Things")
            newT = new_Thing(t["T_TYPE"], (candidates[i][0], candidates[i][1]))
            world_db["Things"][id] = newT
            if (world_db["FAVOR_STAGE"] > 0
                and t["T_TYPE"] == world_db["PLANT_0"]):
                world_db["GOD_FAVOR"] += 5
            elif t["T_TYPE"] == world_db["PLANT_1"]:
                world_db["GOD_FAVOR"] += 25
            elif world_db["FAVOR_STAGE"] >= 4 and \
                t["T_TYPE"] == world_db["ANIMAL_1"]:
                log("The Island God SMILES upon a new-born bear baby.")
                world_db["GOD_FAVOR"] += 750

def make_map():
    global rand

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
    n_colons = int((length ** 2) / 16)
    i_colons = 0
    while (i_colons <= n_colons):
        single_allowed = rand.next() % 256
        y = rand.next() % length
        x = rand.next() % length
        pos = (y * length) + x
        if ("." == chr(world_db["MAP"][pos])
          and ((not single_allowed) or is_neighbor((y, x), ":"))):
            world_db["MAP"][pos] = ord(":")
            i_colons += 1
    altar_placed = False
    while not altar_placed:
        y = rand.next() % length
        x = rand.next() % length
        pos = (y * length) + x
        if (("." == chr(world_db["MAP"][pos]
             or ":" == chr(world_db["MAP"][pos]))
            and not is_neighbor((y, x), "X"))):
            world_db["MAP"][pos] = ord("_")
            world_db["altar"] = (y, x)
            altar_placed = True

def ai(t):
    from server.ai import get_dir_to_target, get_inventory_slot_to_consume, \
        standing_on_food
    t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                      if world_db["ThingActions"][id]["TA_NAME"] == "wait"][0]
    eating = len([id for id in world_db["ThingActions"]
                  if world_db["ThingActions"][id]["TA_NAME"] == "use"]) > 0
    picking = len([id for id in world_db["ThingActions"]
                   if world_db["ThingActions"][id]["TA_NAME"] == "pickup"]) > 0
    if eating and picking:
        if get_dir_to_target(t, "f"):
            return
        sel = get_inventory_slot_to_consume(t)
        if -1 != sel:
            t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                              if world_db["ThingActions"][id]["TA_NAME"]
                                 == "use"][0]
            t["T_ARGUMENT"] = sel
        elif standing_on_food(t) and (len(t["T_CARRIES"]) <
                world_db["ThingTypes"][t["T_TYPE"]]["TT_STORAGE"]):
                t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                                  if world_db["ThingActions"][id]["TA_NAME"]
                                  == "pickup"][0]
        else:
            going_to_known_food_spot = get_dir_to_target(t, "c")
            if not going_to_known_food_spot:
                aiming_for_walking_food = get_dir_to_target(t, "a")
                if not aiming_for_walking_food:
                    get_dir_to_target(t, "s")

def actor_pickup(t):
    from server.ai import eat_vs_hunger_threshold
    used_slots = len(t["T_CARRIES"])
    if used_slots < world_db["ThingTypes"][t["T_TYPE"]]["TT_STORAGE"]:
        ids = [id for id in world_db["Things"] if world_db["Things"][id] != t
               if not world_db["Things"][id]["carried"]
               if world_db["Things"][id]["T_POSY"] == t["T_POSY"]
               if world_db["Things"][id]["T_POSX"] == t["T_POSX"]]
        if len(ids):
            lowest_tid = -1
            eat_cost = eat_vs_hunger_threshold(t["T_TYPE"])
            for iid in ids:
                tid = world_db["Things"][iid]["T_TYPE"] 
                if lowest_tid == -1 or tid < lowest_tid:
                    if (t != world_db["Things"][0] and
                        (world_db["ThingTypes"][tid]["TT_TOOL"] != "food"
                         or (world_db["ThingTypes"][tid]["TT_TOOLPOWER"]
                             <= eat_cost))):
                        continue
                    id = iid
                    lowest_tid = tid
            world_db["Things"][id]["carried"] = True
            ty = world_db["Things"][id]["T_TYPE"]
            if (t != world_db["Things"][0]
                and world_db["Things"][id]["T_PLAYERDROP"]
                and world_db["ThingTypes"][ty]["TT_TOOL"] == "food"):
                score = int(world_db["ThingTypes"][ty]["TT_TOOLPOWER"] / 32)
                world_db["GOD_FAVOR"] += score
                world_db["Things"][id]["T_PLAYERDROP"] = 0
            t["T_CARRIES"].append(id)
            if t == world_db["Things"][0]:
                log("You PICK UP an object.")
    elif t == world_db["Things"][0]:
        log("CAN'T pick up object: No storage room to carry more.")


def actor_drop(t):
    """Make t rop Thing from inventory to ground indexed by T_ARGUMENT."""
    if len(t["T_CARRIES"]):
        id = t["T_CARRIES"][t["T_ARGUMENT"]]
        t["T_CARRIES"].remove(id)
        world_db["Things"][id]["carried"] = False
        if t == world_db["Things"][0]:
            log("You DROP an object.")
            world_db["Things"][id]["T_PLAYERDROP"] = 1


def actor_use(t):
    if len(t["T_CARRIES"]):
        id = t["T_CARRIES"][t["T_ARGUMENT"]]
        type = world_db["Things"][id]["T_TYPE"]
        if type == world_db["SLIPPERS"]:
            if t == world_db["Things"][0]:
                log("You use the " + world_db["ThingTypes"][type]["TT_NAME"]
                    + ". It glows in wondrous colors, and emits a sound as if "
                    + "from a dying cat. The Island God laughs.\n")
            t["T_LIFEPOINTS"] = 1
            from server.config.misc import decrement_lifepoints_func
            decrement_lifepoints_func(t)
        elif (world_db["ThingTypes"][type]["TT_TOOL"] == "carpentry"):
            pos = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
            if (world_db["MAP"][pos] == ord("X")
                or world_db["MAP"][pos] == ord("|")):
                return
            for id in [id for id in world_db["Things"]
                       if not world_db["Things"][id] == t
                       if not world_db["Things"][id]["carried"]
                       if world_db["Things"][id]["T_POSY"] == t["T_POSY"]
                       if world_db["Things"][id]["T_POSX"] == t["T_POSX"]]:
                 return
            wood_id = None
            for id in t["T_CARRIES"]:
                type_material = world_db["Things"][id]["T_TYPE"]
                if (world_db["ThingTypes"][type_material]["TT_TOOL"]
                    == "wood"):
                    wood_id = id
                    break
            if wood_id != None:
                t["T_CARRIES"].remove(wood_id)
                del world_db["Things"][wood_id]
                world_db["MAP"][pos] = ord("|")
                log("With your " + world_db["ThingTypes"][type]["TT_NAME"]
                    + " you build a WOODEN BARRIER from your "
                    + world_db["ThingTypes"][type_material]["TT_NAME"] + ".")
        elif world_db["ThingTypes"][type]["TT_TOOL"] == "fertilizer":
            pos = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
            if world_db["MAP"][pos] == ord("."):
                log("You create SOIL.")
                world_db["MAP"][pos] = ord(":")
        elif world_db["ThingTypes"][type]["TT_TOOL"] == "food":
            t["T_CARRIES"].remove(id)
            del world_db["Things"][id]
            t["T_SATIATION"] += world_db["ThingTypes"][type]["TT_TOOLPOWER"]
            if t == world_db["Things"][0]:
                log("You CONSUME this thing.")
        elif t == world_db["Things"][0]:
            log("You try to use this object, but FAIL.")

def decrement_lifepoints(t):
    t["T_LIFEPOINTS"] -= 1
    live_type = t["T_TYPE"]
    _id = [_id for _id in world_db["Things"] if world_db["Things"][_id] == t][0]
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
            n_species = len([id for id in world_db["Things"]
                             if world_db["Things"][id]["T_TYPE"] == live_type])
            if 0 == n_species:
                from server.new_thing import new_Thing
                if world_db["FAVOR_STAGE"] >= 3 and \
                    live_type == world_db["ANIMAL_0"]:
                    world_db["GOD_FAVOR"] += 3000
                    log("CONGRATULATIONS! The "
                        + world_db["ThingTypes"][live_type]["TT_NAME"]
                        + " species has died out. The Island God is pleased.")
                else:
                    id = id_setter(-1, "Things")
                    world_db["Things"][id] = new_Thing(live_type,
                                                       world_db["altar"])
                    log("The "
                        + world_db["ThingTypes"][live_type]["TT_NAME"]
                        + " species has temporarily died out. "
                        + "One new-born is spawned at the altar.")
        return world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]
    return 0

def actor_move(t):

    def enter_altar():
        from server.new_thing import new_Thing
        if world_db["FAVOR_STAGE"] > 9000:
           log("You step on a soul-less slab of stone.")
           return
        log("YOU ENTER SACRED GROUND.")
        if world_db["FAVOR_STAGE"] == 0:
            world_db["FAVOR_STAGE"] = 1
            log("The Island God speaks to you: \"I don't trust you. You intrud"
                 + "e on the island's affairs. I think you're a nuisance at be"
                 + "st, and a danger to my children at worst. I will give you "
                 + "a chance to lighten my mood, however: For a while now, I'v"
                 + "e been trying to spread the plant "
                 + world_db["ThingTypes"][world_db["PLANT_0"]]["TT_NAME"]
                 + " (\""
                 + world_db["ThingTypes"][world_db["PLANT_0"]]["TT_SYMBOL"]
                 + "\"). I have not been very successful so far. Maybe you can"
                 + " make yourself useful there. I will count each further "
                 + world_db["ThingTypes"][world_db["PLANT_0"]]["TT_NAME"]
                 + " that grows to your favor.\"")
        elif world_db["FAVOR_STAGE"] == 1 and world_db["GOD_FAVOR"] < 100:
            log("The Island God will talk again when it favors you to >=100 "
                 +" points.")
        elif world_db["FAVOR_STAGE"] == 1 and world_db["GOD_FAVOR"] >= 100:
            world_db["FAVOR_STAGE"] = 2
            log("The Island God speaks to you: \"You could have done worse so "
                + "far. Maybe you are not the worst to happen to this island "
                + "since the metal birds threw the great lightning ball. Maybe"
                + " you can help me spread another plant. It multiplies faster"
                + ",and it is highly nutritious: "
                + world_db["ThingTypes"][world_db["PLANT_1"]]["TT_NAME"]
                + " (\""
                + world_db["ThingTypes"][world_db["PLANT_1"]]["TT_SYMBOL"]
                + "\"). It is new. I give you the only example. Be very carefu"
                + "l with it! I also give you another tool that may be helpful"
                + ".\"")
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["PLANT_1"],
                                               world_db["altar"])
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["TOOL_0"],
                                               world_db["altar"])
        elif world_db["FAVOR_STAGE"] == 2 and \
            0 == len([id for id in world_db["Things"]
                      if world_db["Things"][id]["T_TYPE"]
                         == world_db["PLANT_1"]]):
            log("The Island God speaks to you: \"I am greatly disappointed tha"
                + "t you lost all "
                + world_db["ThingTypes"][world_db["PLANT_1"]]["TT_NAME"]
                + " this island had. Here is another one. It cost me great wor"
               + "k. Be more careful this time when planting it.\"")
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["PLANT_1"],
                                               world_db["altar"])
            world_db["GOD_FAVOR"] -= 250
        elif world_db["FAVOR_STAGE"] == 2 and world_db["GOD_FAVOR"] < 500:
            log("The Island God will talk again when it favors you to >=500 "
                 +" points.")
        elif world_db["FAVOR_STAGE"] == 2 and world_db["GOD_FAVOR"] >= 500:
            world_db["FAVOR_STAGE"] = 3
            log("The Island God speaks to you: \"The "
                + world_db["ThingTypes"][world_db["ANIMAL_0"]]["TT_NAME"]
                + " has lately become a pest. These creatures do not please me"
                + " as much as they used to do. Exterminate them all. I will c"
                + "ount each kill to your favor. To help you with the hunting,"
                + " I grant you the empathy and knowledge to read animals.\"")
            log("You will now see animals' health bars, and activities (\"m\": "
                + "moving (maybe for an attack), \"u\": eating, \"p\": picking"
                + " something up; no letter: waiting).")
            world_db["EMPATHY"] = 1
        elif world_db["FAVOR_STAGE"] == 3 and world_db["GOD_FAVOR"] < 5000:
            log("The Island God will talk again when it favors you to >=5000 "
                 +" points.")
        elif world_db["FAVOR_STAGE"] == 3 and world_db["GOD_FAVOR"] >= 5000:
            world_db["FAVOR_STAGE"] = 4
            log("The Island God speaks to you: \"You know what animal I find "
                 + "the cutest? The "
                 + world_db["ThingTypes"][world_db["ANIMAL_1"]]["TT_NAME"]
                 + "! I think what this islands clearly needs more of is "
                 + world_db["ThingTypes"][world_db["ANIMAL_1"]]["TT_NAME"]
                 + "s. Why don't you help? Support them. Make sure they are "
                 + "well, and they will multiply faster. From now on, I will "
                 + "count each new-born "
                 + world_db["ThingTypes"][world_db["ANIMAL_1"]]["TT_NAME"]
                 + " (not spawned by me due to undo an extinction event) "
                 + "greatly to your favor. To help you with the feeding, here "
                 + "is something to make the ground bear more consumables.")
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["TOOL_1"],
                                               world_db["altar"])
        elif world_db["GOD_FAVOR"] < 20000:
            log("The Island God will talk again when it favors you to >=20000 "
                 +" points.")
        elif world_db["GOD_FAVOR"] > 20000:
            world_db["FAVOR_STAGE"] = 9001
            log("The Island God speaks to you: \"You have proven yourself wort"
                 + "hy of my respect. You were a good citizen to the island, a"
                 + "nd sometimes a better steward to its inhabitants than me. "
                 + "The island shall miss you when you leave. But you have ear"
                 + "ned the right to do so. Take this "
                 + world_db["ThingTypes"][world_db["SLIPPERS"]]["TT_NAME"]
                 + " and USE it when you please. It will take you to where you"
                 + " came from. (But do feel free to stay here as long as you "
                 + "like.)\"")
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["SLIPPERS"],
                                               world_db["altar"])

    from server.config.world_data import symbols_passable
    from server.build_fov_map import build_fov_map
    from server.config.misc import decrement_lifepoints_func
    from server.new_thing import new_Thing
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
            hitted_type = world_db["Things"][hit_id]["T_TYPE"]
            if t == world_db["Things"][0]:
                hitted_name = world_db["ThingTypes"][hitted_type]["TT_NAME"]
                log("You WOUND " + hitted_name + ".")
                world_db["GOD_FAVOR"] -= 1
            elif 0 == hit_id:
                hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
                log(hitter_name +" WOUNDS you.")
            test = decrement_lifepoints_func(world_db["Things"][hit_id])
            if test and world_db["FAVOR_STAGE"] >= 3 and \
               hitted_type == world_db["ANIMAL_0"]:
                world_db["GOD_FAVOR"] += 125
            elif test and t == world_db["Things"][0]:
                world_db["GOD_FAVOR"] -= 2 * test
            return
        if (ord("X") == world_db["MAP"][pos]
            or ord("|") == world_db["MAP"][pos]):
            carries_axe = False
            for id in t["T_CARRIES"]:
                type = world_db["Things"][id]["T_TYPE"]
                if world_db["ThingTypes"][type]["TT_TOOL"] == "axe":
                    carries_axe = True
                    break
            if carries_axe:
                axe_name = world_db["ThingTypes"][type]["TT_NAME"]
                if t == world_db["Things"][0]:
                    log("With your " + axe_name + ", you chop!")
                    if ord("X") == world_db["MAP"][pos]:
                        world_db["GOD_FAVOR"] -= 1
                chop_power = world_db["ThingTypes"][type]["TT_TOOLPOWER"]

                case_X = world_db["MAP"][pos] == ord("X")
                if (chop_power > 0
                    and ((case_X and
                          0 == int(rand.next() / chop_power))
                    or (not case_X and
                             0 == int(rand.next() / (3 * chop_power))))):
                    if t == world_db["Things"][0]:
                        log("You chop it DOWN.")
                        if ord("X") == world_db["MAP"][pos]:
                            world_db["GOD_FAVOR"] -= 10
                    world_db["MAP"][pos] = ord(".")
                    i = 3 if case_X else 1
                    for i in range(i):
                        id = id_setter(-1, "Things")
                        world_db["Things"][id] = \
                          new_Thing(world_db["LUMBER"],
                                    (move_result[1], move_result[2]))
                    build_fov_map(t)
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
            if (move_result[1] == world_db["altar"][0] and
                move_result[2] == world_db["altar"][1]):
                enter_altar()

def command_ttid(id_string):
    id = id_setter(id_string, "ThingTypes", command_ttid)
    if None != id:
        world_db["ThingTypes"][id] = {
            "TT_NAME": "(none)",
            "TT_TOOLPOWER": 0,
            "TT_LIFEPOINTS": 0,
            "TT_PROLIFERATE": 0,
            "TT_START_NUMBER": 0,
            "TT_STORAGE": 0,
            "TT_SYMBOL": "?",
            "TT_CORPSE_ID": id,
            "TT_TOOL": ""
        }

def command_worldactive(worldactive_string):
    val = integer_test(worldactive_string, 0, 1)
    if None != val:
        if 0 != world_db["WORLD_ACTIVE"]:
            if 0 == val:
                set_world_inactive()
            else:
                print("World already active.")
        elif 0 == world_db["WORLD_ACTIVE"]:
            wait_exists = False
            for ThingAction in world_db["ThingActions"]:
                if "wait" == world_db["ThingActions"][ThingAction]["TA_NAME"]:
                    wait_exists = True
                    break
            player_exists = False
            for Thing in world_db["Things"]:
                if 0 == Thing:
                    player_exists = True
                    break
            altar_found = False
            if world_db["MAP"]:
                pos = world_db["MAP"].find(b'_')
                if pos > 0:
                    y = int(pos / world_db["MAP_LENGTH"])
                    x = pos % world_db["MAP_LENGTH"]
                    world_db["altar"] = (y, x)
                    altar_found = True
            specials_set = True
            for name in world_db["specials"]:
                if world_db[name] not in world_db["ThingTypes"]:
                    specials_set = False
            if altar_found and wait_exists and player_exists and \
                    world_db["MAP"] and specials_set:
                for id in world_db["Things"]:
                    if world_db["Things"][id]["T_LIFEPOINTS"]:
                        build_fov_map(world_db["Things"][id])
                        if 0 == id:
                            update_map_memory(world_db["Things"][id], False)
                if not world_db["Things"][0]["T_LIFEPOINTS"]:
                    empty_fovmap = bytearray(b" " * world_db["MAP_LENGTH"] ** 2)
                    world_db["Things"][0]["fovmap"] = empty_fovmap
                world_db["WORLD_ACTIVE"] = 1
            else:
                print("Ignoring: Not all conditions for world activation met.")

def play_move(str_arg):
    if action_exists("move"):
        from server.config.world_data import directions_db, symbols_passable
        t = world_db["Things"][0]
        if not str_arg in directions_db:
            print("Illegal move direction string.")
            return
        dir = ord(directions_db[str_arg])
        from server.utils import mv_yx_in_dir_legal
        move_result = mv_yx_in_dir_legal(chr(dir), t["T_POSY"], t["T_POSX"])
        if 1 == move_result[0]:
            pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
            if ord("~") == world_db["MAP"][pos]:
                log("You can't SWIM.")
                return
            if (ord("X") == world_db["MAP"][pos]
                or ord("|") == world_db["MAP"][pos]):
                carries_axe = False
                for id in t["T_CARRIES"]:
                    type = world_db["Things"][id]["T_TYPE"]
                    if world_db["ThingTypes"][type]["TT_TOOL"] == "axe":
                        world_db["Things"][0]["T_ARGUMENT"] = dir
                        set_command("move")
                        return
            if chr(world_db["MAP"][pos]) in symbols_passable:
                world_db["Things"][0]["T_ARGUMENT"] = dir
                set_command("move")
                return
        log("You CAN'T move there.")

def play_use(str_arg):
    if action_exists("use"):
        t = world_db["Things"][0]
        if 0 == len(t["T_CARRIES"]):
            log("You have NOTHING to use in your inventory.")
        else:
            val = integer_test(str_arg, 0, 255)
            if None != val and val < len(t["T_CARRIES"]):
                id = t["T_CARRIES"][val]
                type = world_db["Things"][id]["T_TYPE"]
                if (world_db["ThingTypes"][type]["TT_TOOL"] == "axe"
                      and t == world_db["Things"][0]):
                    log("To use this item for chopping, move towards a tree "
                         + "while carrying it in your inventory.")
                    return
                elif (world_db["ThingTypes"][type]["TT_TOOL"] == "carpentry"):
                    pos = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
                    if (world_db["MAP"][pos] == ord("X")
                        or world_db["MAP"][pos] == ord("|")):
                        log("CAN'T build when standing on barrier.")
                        return
                    for id in [id for id in world_db["Things"]
                               if not world_db["Things"][id] == t
                               if not world_db["Things"][id]["carried"]
                               if world_db["Things"][id]["T_POSY"] == t["T_POSY"]
                               if world_db["Things"][id]["T_POSX"] == t["T_POSX"]]:
                         log("CAN'T build when standing objects.")
                         return
                    wood_id = None
                    for id in t["T_CARRIES"]:
                        type_material = world_db["Things"][id]["T_TYPE"]
                        if (world_db["ThingTypes"][type_material]["TT_TOOL"]
                            == "wood"):
                            wood_id = id
                            break
                    if wood_id == None:
                        log("You CAN'T use a "
                            + world_db["ThingTypes"][type]["TT_NAME"]
                            + " without some wood in your inventory.")
                        return
                elif world_db["ThingTypes"][type]["TT_TOOL"] == "fertilizer":
                    pos = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
                    if not world_db["MAP"][pos] == ord("."):
                        log("Can only make soil out of NON-SOIL earth.")
                        return
                elif world_db["ThingTypes"][type]["TT_TOOL"] == "wood":
                        log("To use wood, you NEED a carpentry tool.")
                        return
                elif type != world_db["SLIPPERS"] and not \
                        world_db["ThingTypes"][type]["TT_TOOL"] == "food":
                    log("You CAN'T consume this thing.")
                    return
                world_db["Things"][0]["T_ARGUMENT"] = val
                set_command("use")
            else:
                print("Illegal inventory index.")

def specialtypesetter(name):
    def helper(str_int):
        val = integer_test(str_int, 0)
        if None != val:
            world_db[name] = val
            if world_db["WORLD_ACTIVE"] \
               and world_db[name] not in world_db["ThingTypes"]:
                world_db["WORLD_ACTIVE"] = 0
                print(name + " fits no known ThingType, deactivating world.")
    return helper

def write_metamap_A():
    from server.worldstate_write_helpers import write_map
    ord_v = ord("v")
    length = world_db["MAP_LENGTH"]
    metamapA = bytearray(b'0' * (length ** 2))
    for id in [id for id in world_db["Things"]
                  if not world_db["Things"][id]["carried"]
                  if world_db["Things"][id]["T_LIFEPOINTS"]
                  if world_db["Things"][0]["fovmap"][
                       world_db["Things"][id]["T_POSY"] * length
                       + world_db["Things"][id]["T_POSX"]] == ord_v]:
        pos = (world_db["Things"][id]["T_POSY"] * length
              + world_db["Things"][id]["T_POSX"])
        if id == 0 or world_db["EMPATHY"]:
            type = world_db["Things"][id]["T_TYPE"]
            max_hp = world_db["ThingTypes"][type]["TT_LIFEPOINTS"]
            third_of_hp = max_hp / 3
            hp = world_db["Things"][id]["T_LIFEPOINTS"]
            add = 0
            if hp > 2 * third_of_hp:
                 add = 2
            elif hp > third_of_hp:
                add = 1
            metamapA[pos] = ord('a') + add
        else:
            metamapA[pos] = ord('X')
    for mt in world_db["Things"][0]["T_MEMTHING"]:
        pos = mt[1] * length + mt[2]
        if metamapA[pos] < ord('2'):
            metamapA[pos] += 1
    return write_map(metamapA, length)

def write_metamap_B():
    from server.worldstate_write_helpers import write_map
    ord_v = ord("v")
    length = world_db["MAP_LENGTH"]
    metamapB = bytearray(b' ' * (length ** 2))
    for id in [id for id in world_db["Things"]
                  if not world_db["Things"][id]["carried"]
                  if world_db["Things"][id]["T_LIFEPOINTS"]
                  if world_db["Things"][0]["fovmap"][
                       world_db["Things"][id]["T_POSY"] * length
                       + world_db["Things"][id]["T_POSX"]] == ord_v]:
        pos = (world_db["Things"][id]["T_POSY"] * length
              + world_db["Things"][id]["T_POSX"])
        if id == 0 or world_db["EMPATHY"]:
            action = world_db["Things"][id]["T_COMMAND"]
            if 0 != action:
                name = world_db["ThingActions"][action]["TA_NAME"]
            else:
                name = " "
            metamapB[pos] = ord(name[0])
    return write_map(metamapB, length)

def calc_effort(thing_action, thing):
    from math import sqrt
    effort = thing_action["TA_EFFORT"]
    if thing_action["TA_NAME"] == "move":
        typ = thing["T_TYPE"]
        max_hp = (world_db["ThingTypes"][typ]["TT_LIFEPOINTS"])
        effort = int(effort / sqrt(max_hp))
        effort = 1 if effort == 0 else effort
    return effort

def play_pickup():
    """Try "pickup" as player's T_COMMAND"."""
    if action_exists("pickup"):
        t = world_db["Things"][0]
        ids = [id for id in world_db["Things"] if id
               if not world_db["Things"][id]["carried"]
               if world_db["Things"][id]["T_POSY"] == t["T_POSY"]
               if world_db["Things"][id]["T_POSX"] == t["T_POSX"]]
        if not len(ids):
            log("NOTHING to pick up.")
        elif len(t["T_CARRIES"]) >= world_db["ThingTypes"][t["T_TYPE"]] \
                ["TT_STORAGE"]:
            log("CAN'T pick up: No storage room to carry anything more.")
        else:
            set_command("pickup")

strong_write(io_db["file_out"], "PLUGIN PleaseTheIslandGod\n")

if not "GOD_FAVOR" in world_db:
    world_db["GOD_FAVOR"] = 0
if not "FAVOR_STAGE" in world_db:
    world_db["FAVOR_STAGE"] = 0
if not "SLIPPERS" in world_db:
    world_db["SLIPPERS"] = 0
if not "PLANT_0" in world_db:
    world_db["PLANT_0"] = 0
if not "PLANT_1" in world_db:
    world_db["PLANT_1"] = 0
if not "ANIMAL_0" in world_db:
    world_db["ANIMAL_0"] = 0
if not "ANIMAL_1" in world_db:
    world_db["ANIMAL_1"] = 0
if not "TOOL_0" in world_db:
    world_db["TOOL_0"] = 0
if not "TOOL_1" in world_db:
    world_db["TOOL_1"] = 0
if not "LUMBER" in world_db:
    world_db["LUMBER"] = 0
if not "EMPATHY" in world_db:
    world_db["EMPATHY"] = 0
world_db["terrain_names"][":"] = "SOIL"
world_db["terrain_names"]["|"] = "WALL"
world_db["terrain_names"]["_"] = "ALTAR"
world_db["specials"] = ["SLIPPERS", "PLANT_0", "PLANT_1", "TOOL_0", "TOOL_1",
    "LUMBER", "ANIMAL_0, ANIMAL_1"]
io_db["worldstate_write_order"] += [["GOD_FAVOR", "world_int"]]
io_db["worldstate_write_order"] += [[write_metamap_A, "func"]]
io_db["worldstate_write_order"] += [[write_metamap_B, "func"]]

import server.config.world_data
server.config.world_data.symbols_passable += ":_"

from server.config.world_data import thing_defaults
thing_defaults["T_PLAYERDROP"] = 0

import server.config.actions
server.config.actions.action_db["actor_move"] = actor_move
server.config.actions.action_db["actor_pickup"] = actor_pickup
server.config.actions.action_db["actor_drop"] = actor_drop
server.config.actions.action_db["actor_use"] = actor_use
server.config.actions.ai_func = ai

from server.config.commands import commands_db
commands_db["TT_ID"] = (1, False, command_ttid)
commands_db["GOD_FAVOR"] = (1, False, setter(None, "GOD_FAVOR", -32768, 32767))
commands_db["TT_STORAGE"] = (1, False, setter("ThingType", "TT_STORAGE", 0, 255))
commands_db["T_PLAYERDROP"] = (1, False, setter("Thing", "T_PLAYERDROP", 0, 1))
commands_db["WORLD_ACTIVE"] = (1, False, command_worldactive)
commands_db["FAVOR_STAGE"] = (1, False, setter(None, "FAVOR_STAGE", 0, 1))
commands_db["SLIPPERS"] = (1, False, specialtypesetter("SLIPPERS"))
commands_db["TOOL_0"] = (1, False, specialtypesetter("TOOL_0"))
commands_db["TOOL_1"] = (1, False, specialtypesetter("TOOL_1"))
commands_db["ANIMAL_0"] = (1, False, specialtypesetter("ANIMAL_0"))
commands_db["ANIMAL_1"] = (1, False, specialtypesetter("ANIMAL_1"))
commands_db["PLANT_0"] = (1, False, specialtypesetter("PLANT_0"))
commands_db["PLANT_1"] = (1, False, specialtypesetter("PLANT_1"))
commands_db["LUMBER"] = (1, False, specialtypesetter("LUMBER"))
commands_db["EMPATHY"] = (1, False, setter(None, "EMPATHY", 0, 1))
commands_db["use"] = (1, False, play_use)
commands_db["move"] = (1, False, play_move)
commands_db["pickup"] = (0, False, play_pickup)

import server.config.misc
server.config.misc.make_map_func = make_map
server.config.misc.thingproliferation_func = thingproliferation
server.config.misc.make_world = make_world
server.config.misc.decrement_lifepoints_func = decrement_lifepoints
server.config.misc.calc_effort_func = calc_effort
