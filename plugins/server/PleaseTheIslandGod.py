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
    if not world_db["SLIPPERS"] in world_db["ThingTypes"]:
        print("Ignoring: No valid SLIPPERS set.")
        return
    if not world_db["PLANT_0"] in world_db["ThingTypes"]:
        print("Ignoring: No valid PLANT_0 set.")
        return
    #for name in specials:
    #    if world_db[name] not in world_db["ThingTypes"]:
    #        print("Ignoring: No valid " + name + " set.")
    #        return
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
            #elif t["T_TYPE"] == world_db["PLANT_1"];
            #    world_db["GOD_FAVOR"] += 25
            #elif world_db["FAVOR_STAGE"] >= 4 and \
            #     t["T_TYPE"] == world_db["ANIMAL_1"]:
            #    log("The Island God SMILES upon a new-born bear baby.")
            #    world_db["GOD_FAVOR"] += 750

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
        log("Can't pick up object: No storage room to carry more.")


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
        elif world_db["ThingTypes"][type]["TT_TOOL"] == "food":
            t["T_CARRIES"].remove(id)
            del world_db["Things"][id]
            t["T_SATIATION"] += world_db["ThingTypes"][type]["TT_TOOLPOWER"]
            if t == world_db["Things"][0]:
                log("You CONSUME this object.")
        elif t == world_db["Things"][0]:
            log("You try to use this object, but FAIL.")

def decrement_lifepoints(t):
    t["T_LIFEPOINTS"] -= 1
    _id = [_id for _id in world_db["Things"] if world_db["Things"][_id] == t][0]
    if 0 == t["T_LIFEPOINTS"]:
        sadness = world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]
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
        return sadness
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
        elif world_db["GOD_FAVOR"] > 150:
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
                world_db["GOD_FAVOR"] -= 1
            elif 0 == hit_id:
                hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
                log(hitter_name +" WOUNDS you.")
            test = decrement_lifepoints_func(world_db["Things"][hit_id])
            if test and t == world_db["Things"][0]:
                world_db["GOD_FAVOR"] -= test 
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
            valid_slippers = world_db["SLIPPERS"] in world_db["ThingTypes"]
            valid_plant0 = world_db["PLANT_0"] in world_db["ThingTypes"]
            if altar_found and wait_exists and player_exists and \
                    world_db["MAP"] and valid_slippers and valid_plant0:
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

def command_slippers(str_int):
    val = integer_test(str_int, 0)
    if None != val:
        world_db["SLIPPERS"] = val
        if world_db["WORLD_ACTIVE"] and \
           world_db["SLIPPERS"] not in world_db["ThingTypes"]:
            world_db["WORLD_ACTIVE"] = 0
            print("SLIPPERS matches no known ThingTypes, deactivating world.")

def command_plant0(str_int):
    val = integer_test(str_int, 0)
    if None != val:
        world_db["PLANT_0"] = val
        if world_db["WORLD_ACTIVE"] and \
           world_db["PLANT_0"] not in world_db["ThingTypes"]:
            world_db["WORLD_ACTIVE"] = 0
            print("PLANT_0 matches no known ThingTypes, deactivating world.")

def play_use(str_arg):
    """Try "use" as player's T_COMMAND, int(str_arg) as T_ARGUMENT / slot."""
    if action_exists("use"):
        t = world_db["Things"][0]
        if 0 == len(t["T_CARRIES"]):
            log("You have NOTHING to use in your inventory.")
        else:
            val = integer_test(str_arg, 0, 255)
            if None != val and val < len(t["T_CARRIES"]):
                id = t["T_CARRIES"][val]
                type = world_db["Things"][id]["T_TYPE"]
                if type != world_db["SLIPPERS"] and not \
                        world_db["ThingTypes"][type]["TT_TOOL"] == "food":
                    log("You CAN'T consume this thing.")
                    return
                world_db["Things"][0]["T_ARGUMENT"] = val
                set_command("use")
            else:
                print("Illegal inventory index.")

strong_write(io_db["file_out"], "PLUGIN PleaseTheIslandGod\n")

if not "GOD_FAVOR" in world_db:
    world_db["GOD_FAVOR"] = 0
if not "FAVOR_STAGE" in world_db:
    world_db["FAVOR_STAGE"] = 0
if not "SLIPPERS" in world_db:
    world_db["SLIPPERS"] = 0
if not "PLANT_0" in world_db:
    world_db["PLANT_0"] = 0
io_db["worldstate_write_order"] += [["GOD_FAVOR", "world_int"]]

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
commands_db["SLIPPERS"] = (1, False, command_slippers)
commands_db["PLANT_0"] = (1, False, command_plant0)
commands_db["use"] = (1, False, play_use)

import server.config.misc
server.config.misc.make_map_func = make_map
server.config.misc.thingproliferation_func = thingproliferation
server.config.misc.make_world = make_world
server.config.decrement_lifepoints_func = decrement_lifepoints
