# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.io import log, strong_write
from server.config.world_data import world_db, directions_db
from server.utils import mv_yx_in_dir_legal, rand, id_setter
from server.config.io import io_db

def pos_test(type, y, x):
    pos = y * world_db["MAP_LENGTH"] + x;
    plant = world_db["ThingTypes"][type]["TT_PROLIFERATE"]
    return (not plant or ":" == chr(world_db["MAP"][pos]))

def world_makable():
    from server.world_makable import world_makable
    playertype = world_makable()
    for name in world_db["specials"]:
        if world_db[name] not in world_db["ThingTypes"]:
            print("Ignoring: No valid " + name + " set.")
            return -1
    return playertype

def make_map():
    from server.make_map import make_map, is_neighbor, new_pos
    global rand
    make_map()
    length = world_db["MAP_LENGTH"]
    n_colons = int((length ** 2) / 16)
    i_colons = 0
    while (i_colons <= n_colons):
        single_allowed = rand.next() % 256
        y, x, pos = new_pos()
        if ("." == chr(world_db["MAP"][pos])
          and ((not single_allowed) or is_neighbor((y, x), ":"))):
            world_db["MAP"][pos] = ord(":")
            i_colons += 1
    altar_placed = False
    while not altar_placed:
        y, x, pos = new_pos()
        if (("." == chr(world_db["MAP"][pos]
             or ":" == chr(world_db["MAP"][pos]))
            and not is_neighbor((y, x), "X"))):
            world_db["MAP"][pos] = ord("_")
            world_db["altar"] = (y, x)
            altar_placed = True

def field_spreadable(c, t):
    return ":" == c or (world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]
                        and "." == c)

def thingprol_plugin_conditions(t):
    tt = world_db["ThingTypes"][t["T_TYPE"]]
    return (tt["TT_LIFEPOINTS"] == 0 or \
            t["T_LIFEPOINTS"] >= 0.9 * tt["TT_LIFEPOINTS"])

def thingprol_plugin_post_create_hook(t):
    tt = world_db["ThingTypes"][t["T_TYPE"]]
    if (world_db["FAVOR_STAGE"] > 0 and t["T_TYPE"] == world_db["PLANT_0"]):
        world_db["GOD_FAVOR"] += 5
    elif t["T_TYPE"] == world_db["PLANT_1"]:
        world_db["GOD_FAVOR"] += 25
    elif world_db["FAVOR_STAGE"] >= 4 and t["T_TYPE"] == world_db["ANIMAL_1"]:
        log("The Island God SMILES upon a new-born bear baby.")
        world_db["GOD_FAVOR"] += 750

def ai_hook_pickup_test(t):
    return len(t["T_CARRIES"]) < \
        world_db["ThingTypes"][t["T_TYPE"]]["TT_STORAGE"]

def actor_pickup(t):
    used_slots = len(t["T_CARRIES"])
    if used_slots < world_db["ThingTypes"][t["T_TYPE"]]["TT_STORAGE"]:
        from server.actions import actor_pickup
        t_picked = actor_pickup(t)
        if t_picked != None:
            ty = world_db["ThingTypes"][t_picked["T_TYPE"]]
            if t != world_db["Things"][0] and t_picked["T_PLAYERDROP"] \
                    and ty["TT_TOOL"] == "food":
                score = int(ty["TT_TOOLPOWER"] / 32)
                world_db["GOD_FAVOR"] += score
                t_picked["T_PLAYERDROP"] = 0
    elif t == world_db["Things"][0]:
        log("CAN'T pick up object: No storage room to carry more.")

def actor_pickup_test_hook(t, tid):
    from server.ai import eat_vs_hunger_threshold
    tt = world_db["ThingTypes"][tid]
    return not (t != world_db["Things"][0] and (tt["TT_TOOL"] != "food" or
            (tt["TT_TOOLPOWER"] <= eat_vs_hunger_threshold(t["T_TYPE"]))))

def actor_drop(t):
    from server.actions import actor_drop
    dropped = actor_drop(t)
    if dropped != None:
        dropped["T_PLAYERDROP"] = 1

def actor_use_attempts_hook(t, ty):
    if ty == world_db["SLIPPERS"]:
        if t == world_db["Things"][0]:
            log("You use the " + world_db["ThingTypes"][ty]["TT_NAME"] + ". " \
                "It glows in wondrous colors, and emits a sound as if from a d"
                "ying cat. The Island God laughs.\n")
        t["T_LIFEPOINTS"] = 1
        from server.config.misc import decrement_lifepoints_func
        decrement_lifepoints_func(t)
    elif (world_db["ThingTypes"][ty]["TT_TOOL"] == "carpentry"):
        pos = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
        if (world_db["MAP"][pos] == ord("X")
            or world_db["MAP"][pos] == ord("|")):
            return
        for t_id in [t_id for t_id in world_db["Things"]
                   if not world_db["Things"][t_id] == t
                   if not world_db["Things"][t_id]["carried"]
                   if world_db["Things"][t_id]["T_POSY"] == t["T_POSY"]
                   if world_db["Things"][t_id]["T_POSX"] == t["T_POSX"]]:
            return
        wood_id = None
        for t_id in t["T_CARRIES"]:
            type_material = world_db["Things"][t_id]["T_TYPE"]
            if (world_db["ThingTypes"][type_material]["TT_TOOL"] == "wood"):
                wood_id = t_id
                break
        if wood_id != None:
            t["T_CARRIES"].remove(wood_id)
            del world_db["Things"][wood_id]
            world_db["MAP"][pos] = ord("|")
            log("With your " + world_db["ThingTypes"][ty]["TT_NAME"] + " you" \
                " build a WOODEN BARRIER from your "
                + world_db["ThingTypes"][type_material]["TT_NAME"] + ".")
    elif world_db["ThingTypes"][ty]["TT_TOOL"] == "fertilizer":
        pos = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
        if world_db["MAP"][pos] == ord("."):
            log("You create SOIL.")
            world_db["MAP"][pos] = ord(":")

def actor_move(t):

    def altar_msg_wait(limit):
            log("The Island God will talk again when it favors you to >=" +
                str(limit) + " points.")

    altar_msg_0 = "The Island God speaks to you: \"I don't trust you. You in" \
    "trude on the island's affairs. I think you're a nuisance at best, and a" \
    " danger to my children at worst. I will give you a chance to lighten my" \
    " mood, however: For a while now, I've been trying to spread the plant " \
    + world_db["ThingTypes"][world_db["PLANT_0"]]["TT_NAME"] + " (\"" + \
    world_db["ThingTypes"][world_db["PLANT_0"]]["TT_SYMBOL"] + "\"). I have " \
    "not been very successful so far. Maybe you can make yourself useful the" \
    "re. I will count each further " + \
    world_db["ThingTypes"][world_db["PLANT_0"]]["TT_NAME"] + " that grows to" \
    " your favor.\""

    altar_msg_1 = "The Island God speaks to you: \"You could have done worse" \
    " so far. Maybe you are not the worst to happen to this island since the" \
    " metal birds threw the great lightning ball. Maybe you can help me spre" \
    "ad another plant. It multiplies faster,and it is highly nutritious: " + \
    world_db["ThingTypes"][world_db["PLANT_1"]]["TT_NAME"] + " (\"" + \
    world_db["ThingTypes"][world_db["PLANT_1"]]["TT_SYMBOL"] + "\"). It is n" \
    "ew. I give you the only example. Be very careful with it! I also give y" \
    "ou another tool that may be helpful.\""

    altar_msg_2 = "The Island God speaks to you: \"I am greatly disappointed" \
    " that you lost all " + \
    world_db["ThingTypes"][world_db["PLANT_1"]]["TT_NAME"] + " this island h" \
    "ad. Here is another one. It cost me great work. Be more careful this ti" \
    "me when planting it.\""

    altar_msg_3 = "The Island God speaks to you: \"The " + \
    world_db["ThingTypes"][world_db["ANIMAL_0"]]["TT_NAME"] + " has lately b" \
    "ecome a pest. These creatures do not please me as much as they used to " \
    "do. Exterminate them all. I will count each kill to your favor. To help" \
    " you with the hunting, I grant you the empathy and knowledge to read an" \
    "imals.\""

    altar_msg_4 = "You will now see animals' health bars, and activities (\"" \
    "m\": moving (maybe for an attack), \"u\": eating, \"p\": picking someth" \
    "ing up; no letter: waiting)."

    altar_msg_5 = "The Island God speaks to you: \"You know what animal I fi" \
    "nd the cutest? The " + \
    world_db["ThingTypes"][world_db["ANIMAL_1"]]["TT_NAME"] + "! I think wha" \
    "t this islands clearly needs more of is " + \
    world_db["ThingTypes"][world_db["ANIMAL_1"]]["TT_NAME"] + "s. Why don't " \
    "you help? Support them. Make sure they are well, and they will multiply" \
    " faster. From now on, I will count each new-born " + \
    world_db["ThingTypes"][world_db["ANIMAL_1"]]["TT_NAME"] + \
    " (not spawned by me due to undo an extinction event) greatly to your fa" \
    "vor. To help you with the feeding, here is something to make the ground" \
    " bear more consumables."

    altar_msg_6 = "The Island God speaks to you: \"You have proven yourself " \
    "worthy of my respect. You were a good citizen to the island, and someti" \
    "mes a better steward to its inhabitants than me. The island shall miss " \
    "you when you leave. But you have earned the right to do so. Take this" + \
    world_db["ThingTypes"][world_db["SLIPPERS"]]["TT_NAME"] + " and USE it w" \
    "hen you please. It will take you to where you came from. (But do feel f" \
    "ree to stay here as long as you like.)\""

    def enter_altar():
        from server.new_thing import new_Thing
        if world_db["FAVOR_STAGE"] > 9000:
           log("You step on a soul-less slab of stone.")
           return
        log("YOU ENTER SACRED GROUND.")
        if world_db["FAVOR_STAGE"] == 0:
            world_db["FAVOR_STAGE"] = 1
            log(altar_msg_0)
        elif world_db["FAVOR_STAGE"] == 1 and world_db["GOD_FAVOR"] < 100:
            altar_msg_wait(100)
        elif world_db["FAVOR_STAGE"] == 1 and world_db["GOD_FAVOR"] >= 100:
            world_db["FAVOR_STAGE"] = 2
            log(altar_msg_2)
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
            log(altar_msg_2)
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["PLANT_1"],
                                               world_db["altar"])
            world_db["GOD_FAVOR"] -= 250
        elif world_db["FAVOR_STAGE"] == 2 and world_db["GOD_FAVOR"] < 500:
            altar_msg_wait(500)
        elif world_db["FAVOR_STAGE"] == 2 and world_db["GOD_FAVOR"] >= 500:
            world_db["FAVOR_STAGE"] = 3
            log(altar_msg_3)
            log(altar_msg_4)
            world_db["EMPATHY"] = 1
        elif world_db["FAVOR_STAGE"] == 3 and world_db["GOD_FAVOR"] < 5000:
            altar_msg_wait(5000)
        elif world_db["FAVOR_STAGE"] == 3 and world_db["GOD_FAVOR"] >= 5000:
            world_db["FAVOR_STAGE"] = 4
            log(altar_msg_5)
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["TOOL_1"],
                                               world_db["altar"])
        elif world_db["GOD_FAVOR"] < 20000:
            altar_msg_wait(20000)
        elif world_db["GOD_FAVOR"] > 20000:
            world_db["FAVOR_STAGE"] = 9001
            log(altar_msg_6)
            id = id_setter(-1, "Things")
            world_db["Things"][id] = new_Thing(world_db["SLIPPERS"],
                                               world_db["altar"])

    from server.actions import actor_move
    test = actor_move(t)
    if 2 == len(test):
        if test[0] > 0:
            if world_db["FAVOR_STAGE"] >= 3 and \
                    test[1] == world_db["ANIMAL_0"]:
                world_db["GOD_FAVOR"] += 125
            elif t == world_db["Things"][0]:
                world_db["GOD_FAVOR"] -= 2 * test[1]
        elif t == world_db["Things"][0]:
            world_db["GOD_FAVOR"] -= 1
    elif 3 == len(test):
        if (t == world_db["Things"][0] and
                test[1] == world_db["altar"][0] and
                test[2] == world_db["altar"][1]):
            enter_altar()

def actor_move_attempts_hook(t, move_result, pos):
    if (ord("X") == world_db["MAP"][pos] or ord("|") == world_db["MAP"][pos]):
        for tid in t["T_CARRIES"]:
            ty = world_db["Things"][tid]["T_TYPE"]
            if world_db["ThingTypes"][ty]["TT_TOOL"] == "axe":
                axe_name = world_db["ThingTypes"][ty]["TT_NAME"]
                if t == world_db["Things"][0]:
                    log("With your " + axe_name + ", you chop!")
                    if ord("X") == world_db["MAP"][pos]:
                        world_db["GOD_FAVOR"] -= 1
                chop_power = world_db["ThingTypes"][ty]["TT_TOOLPOWER"]
                case_X = world_db["MAP"][pos] == ord("X")
                if (chop_power > 0 and
                    ((case_X and 0 == int(rand.next() / chop_power))
                     or (not case_X and
                         0 == int(rand.next() / (3 * chop_power))))):
                    if t == world_db["Things"][0]:
                        log("You chop it DOWN.")
                        if ord("X") == world_db["MAP"][pos]:
                            world_db["GOD_FAVOR"] -= 10
                    world_db["MAP"][pos] = ord(".")
                    i = 3 if case_X else 1
                    for i in range(i):
                        tid = id_setter(-1, "Things")
                        world_db["Things"][tid] = \
                          new_Thing(world_db["LUMBER"],
                                    (move_result[1], move_result[2]))
                    build_fov_map(t)
                return True
    return False

def decrement_lifepoints(t):
    t["T_LIFEPOINTS"] -= 1
    live_tid = t["T_TYPE"]
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
                             if world_db["Things"][id]["T_TYPE"] == live_tid])
            if 0 == n_species:
                from server.new_thing import new_Thing
                if world_db["FAVOR_STAGE"] >= 3 and \
                    live_tid == world_db["ANIMAL_0"]:
                    world_db["GOD_FAVOR"] += 3000
                    log("CONGRATULATIONS! The "
                        + world_db["ThingTypes"][live_tid]["TT_NAME"]
                        + " species has died out. The Island God is pleased.")
                else:
                    id = id_setter(-1, "Things")
                    world_db["Things"][id] = new_Thing(live_tid,
                                                       world_db["altar"])
                    log("The "
                        + world_db["ThingTypes"][live_tid]["TT_NAME"]
                        + " species has temporarily died out. "
                        + "One new-born is spawned at the altar.")
        return world_db["ThingTypes"][live_tid]["TT_LIFEPOINTS"]
    return 0

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
            for ThingAction in world_db["ThingActions"]:
                if "wait" == world_db["ThingActions"][ThingAction]["TA_NAME"]:
                    break
            else:
                print("Ignored: No wait action defined for world to activate.")
                return
            for Thing in world_db["Things"]:
                if 0 == Thing:
                    break
            else:
                print("Ignored: No player defined for world to activate.")
                return
            if world_db["MAP"]:
                pos = world_db["MAP"].find(b'_')
                if pos > 0:
                    y = int(pos / world_db["MAP_LENGTH"])
                    x = pos % world_db["MAP_LENGTH"]
                    world_db["altar"] = (y, x)
                else:
                    print("Ignored: No altar defined for world to activate.")
                    return
            else:
                print("Ignored: No map defined for world to activate.")
                return
            for name in world_db["specials"]:
                if world_db[name] not in world_db["ThingTypes"]:
                    print("Ignored: Not all specials set for world to "
                          "activate.")
                    return
            for id in world_db["Things"]:
                if world_db["Things"][id]["T_LIFEPOINTS"]:
                    build_fov_map(world_db["Things"][id])
                    if 0 == id:
                        update_map_memory(world_db["Things"][id], False)
            if not world_db["Things"][0]["T_LIFEPOINTS"]:
                empty_fovmap = bytearray(b" " * world_db["MAP_LENGTH"] ** 2)
                world_db["Things"][0]["fovmap"] = empty_fovmap
            world_db["WORLD_ACTIVE"] = 1

def play_move(str_arg):
    if action_exists("move"):
        from server.config.world_data import directions_db, symbols_passable
        t = world_db["Things"][0]
        if not str_arg in directions_db:
            print("Illegal move direction string.")
            return
        dir = ord(directions_db[str_arg])
        global mv_yx_in_dir_legal
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
                        "while carrying it in your inventory.")
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

def set_zero_if_undefined(key):
    if not key in world_db:
        world_db[key] = 0

set_zero_if_undefined("GOD_FAVOR")
set_zero_if_undefined("FAVOR_STAGE")
set_zero_if_undefined("EMPATHY")
world_db["specials"] = ["SLIPPERS", "PLANT_0", "PLANT_1", "TOOL_0", "TOOL_1",
    "LUMBER", "ANIMAL_0", "ANIMAL_1"]
for key in world_db["specials"]:
    set_zero_if_undefined(key)

world_db["terrain_names"][":"] = "SOIL"
world_db["terrain_names"]["|"] = "WALL"
world_db["terrain_names"]["_"] = "ALTAR"
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
server.config.actions.actor_pickup_test_hook = actor_pickup_test_hook
server.config.actions.actor_use_attempts_hook = actor_use_attempts_hook
server.config.actions.actor_move_attempts_hook = actor_move_attempts_hook

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
server.config.misc.decrement_lifepoints_func = decrement_lifepoints
server.config.misc.calc_effort_func = calc_effort

import server.config.make_world_helpers
server.config.make_world_helpers.pos_test_func = pos_test
server.config.make_world_helpers.world_makable_func = world_makable
server.config.make_world_helpers.make_map_func = make_map

import server.config.thingproliferation
server.config.thingproliferation.field_spreadable = field_spreadable
server.config.thingproliferation.thingprol_plugin_conditions = \
    thingprol_plugin_conditions
server.config.thingproliferation.thingprol_plugin_post_create_hook = \
    thingprol_plugin_post_create_hook

import server.config.ai
server.config.ai.ai_hook_pickup = ai_hook_pickup_test
