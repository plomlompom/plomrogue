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

def thingprol_field_spreadable(c, t):
    return ":" == c or (world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]
                        and "." == c)

def thingprol_test(t):
    tt = world_db["ThingTypes"][t["T_TYPE"]]
    return (tt["TT_LIFEPOINTS"] == 0 or \
            t["T_LIFEPOINTS"] >= 0.9 * tt["TT_LIFEPOINTS"])

def thingprol_post_create(t):
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
    tt = world_db["ThingTypes"][tid]
    return not (t != world_db["Things"][0] and (tt["TT_TOOL"] != "food" or
            (tt["TT_TOOLPOWER"] <=
             world_db["ThingTypes"][t["T_TYPE"]]["eat_vs_hunger_threshold"])))

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
                "ying cat. The Island God laughs.")
        t["T_LIFEPOINTS"] = 1
        from server.config.misc import decrement_lifepoints
        decrement_lifepoints(t)
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
            log(altar_msg_1)
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
    if test != None:
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
                    from server.new_thing import new_Thing
                    for i in range(i):
                        tid = id_setter(-1, "Things")
                        world_db["Things"][tid] = new_Thing(world_db["LUMBER"],
                                    (move_result[1], move_result[2]))
                    build_fov_map(t)
                return True
    return False

def decrement_lifepoints(t):
    from server.decrement_lifepoints import decrement_lifepoints
    live_tid = t["T_TYPE"]
    test = decrement_lifepoints(t)
    if test > 0 and t != world_db["Things"][0]:
        n_species = len([tid for tid in world_db["Things"]
                         if world_db["Things"][tid]["T_TYPE"] == live_tid])
        if 0 == n_species:
            from server.new_thing import new_Thing
            if world_db["FAVOR_STAGE"] >= 3 and \
                    live_tid == world_db["ANIMAL_0"]:
                world_db["GOD_FAVOR"] += 3000
                log("CONGRATULATIONS! The "
                    + world_db["ThingTypes"][live_tid]["TT_NAME"]
                    + " species has died out. The Island God is pleased.")
            else:
                tid = id_setter(-1, "Things")
                world_db["Things"][tid] = new_Thing(live_tid,
                                                    world_db["altar"])
                log("The " + world_db["ThingTypes"][live_tid]["TT_NAME"] + " s"
                    "pecies has temporarily died out. One new-born is spawned "
                    "at the altar.")
    return test

def command_worldactive_test_hook():
    pos = world_db["MAP"].find(b'_')
    if pos > 0:
        y = int(pos / world_db["MAP_LENGTH"])
        x = pos % world_db["MAP_LENGTH"]
        world_db["altar"] = (y, x)
    else:
        print("Ignored: No altar defined for world to activate.")
        return False
    for name in world_db["specials"]:
        if world_db[name] not in world_db["ThingTypes"]:
            print("Ignored: Not all specials set for world to activate.")
            return False
    return True

def play_move_attempt_hook(t, direction, pos):
    if (ord("X") == world_db["MAP"][pos] or ord("|") == world_db["MAP"][pos]):
        carries_axe = False
        for tid in t["T_CARRIES"]:
            ty = world_db["Things"][tid]["T_TYPE"]
            if world_db["ThingTypes"][ty]["TT_TOOL"] == "axe":
                world_db["Things"][0]["T_ARGUMENT"] = direction
                set_command("move")
                return True
    return False

def play_use_attempt_hook(t, tt):
    pos = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
    if tt["TT_TOOL"] == "axe":
        log("To use this item for chopping, MOVE towards a tree while carrying"
            " it in your inventory.")
        return False
    elif tt["TT_TOOL"] == "carpentry":
        if (world_db["MAP"][pos] == ord("X")
            or world_db["MAP"][pos] == ord("|")):
            log("CAN'T build when standing on barrier.")
            return False
        for tid in [tid for tid in world_db["Things"]
                    if not world_db["Things"][tid] == t
                    if not world_db["Things"][tid]["carried"]
                    if world_db["Things"][tid]["T_POSY"] == t["T_POSY"]
                    if world_db["Things"][tid]["T_POSX"] == t["T_POSX"]]:
             log("CAN'T build when standing objects.")
             return False
        wood_id = None
        for tid in t["T_CARRIES"]:
            type_material = world_db["Things"][tid]["T_TYPE"]
            if world_db["ThingTypes"][type_material]["TT_TOOL"] == "wood":
                wood_id = tid
                break
        if wood_id == None:
            log("You CAN'T use a " + tt["TT_NAME"]
                + " without some wood in your inventory.")
            return False
        return True
    elif tt["TT_TOOL"] == "fertilizer":
        if not world_db["MAP"][pos] == ord("."):
            log("Can only make soil out of NON-SOIL earth.")
            return False
        return True
    elif tt["TT_TOOL"] == "wood":
        log("To use wood, you NEED a carpentry tool.")
        return False
    elif tt == world_db["ThingTypes"][world_db["SLIPPERS"]]:
        return True

def play_pickup_attempt_hook(t):
    if len(t["T_CARRIES"]) >= world_db["ThingTypes"][t["T_TYPE"]]["TT_STORAGE"]:
        log("CAN'T pick up: No storage room to carry anything more.")
        return False
    return True

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
    for tid in [tid for tid in world_db["Things"]
                  if not world_db["Things"][tid]["carried"]
                  if world_db["Things"][tid]["T_LIFEPOINTS"]
                  if world_db["Things"][0]["fovmap"][
                       world_db["Things"][tid]["pos"]] == ord_v]:
        pos = (world_db["Things"][tid]["pos"])
        if tid == 0 or world_db["EMPATHY"]:
            ttid = world_db["Things"][tid]["T_TYPE"]
            max_hp = world_db["ThingTypes"][ttid]["TT_LIFEPOINTS"]
            third_of_hp = max_hp / 3
            hp = world_db["Things"][tid]["T_LIFEPOINTS"]
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
    for tid in [tid for tid in world_db["Things"]
                  if not world_db["Things"][tid]["carried"]
                  if world_db["Things"][tid]["T_LIFEPOINTS"]
                  if world_db["Things"][0]["fovmap"][
                       world_db["Things"][tid]["pos"]] == ord_v]:
        pos = (world_db["Things"][tid]["pos"])
        if tid == 0 or world_db["EMPATHY"]:
            action = world_db["Things"][tid]["T_COMMAND"]
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
server.config.world_data.thingprol_field_spreadable = thingprol_field_spreadable
server.config.world_data.thingprol_test_hook = thingprol_test
server.config.world_data.thingprol_post_create_hook = thingprol_post_create

from server.config.world_data import thing_defaults, thingtype_defaults
thing_defaults["T_PLAYERDROP"] = 0
thingtype_defaults["TT_STORAGE"] = 0

import server.config.actions
server.config.actions.action_db["actor_move"] = actor_move
server.config.actions.action_db["actor_pickup"] = actor_pickup
server.config.actions.action_db["actor_drop"] = actor_drop
server.config.actions.actor_pickup_test_hook = actor_pickup_test_hook
server.config.actions.actor_use_attempts_hook = actor_use_attempts_hook
server.config.actions.actor_move_attempts_hook = actor_move_attempts_hook

from server.config.commands import commands_db
commands_db["GOD_FAVOR"] = (1, False, setter(None, "GOD_FAVOR", -32768, 32767))
commands_db["TT_STORAGE"] = (1, False, setter("ThingType", "TT_STORAGE", 0, 255))
commands_db["T_PLAYERDROP"] = (1, False, setter("Thing", "T_PLAYERDROP", 0, 1))
commands_db["FAVOR_STAGE"] = (1, False, setter(None, "FAVOR_STAGE", 0, 255))
commands_db["SLIPPERS"] = (1, False, specialtypesetter("SLIPPERS"))
commands_db["TOOL_0"] = (1, False, specialtypesetter("TOOL_0"))
commands_db["TOOL_1"] = (1, False, specialtypesetter("TOOL_1"))
commands_db["ANIMAL_0"] = (1, False, specialtypesetter("ANIMAL_0"))
commands_db["ANIMAL_1"] = (1, False, specialtypesetter("ANIMAL_1"))
commands_db["PLANT_0"] = (1, False, specialtypesetter("PLANT_0"))
commands_db["PLANT_1"] = (1, False, specialtypesetter("PLANT_1"))
commands_db["LUMBER"] = (1, False, specialtypesetter("LUMBER"))
commands_db["EMPATHY"] = (1, False, setter(None, "EMPATHY", 0, 1))
import server.config.commands
server.config.commands.command_worldactive_test_hook = \
    command_worldactive_test_hook
server.config.commands.play_move_attempt_hook = play_move_attempt_hook
server.config.commands.play_use_attempt_hook = play_use_attempt_hook
server.config.commands.play_pickup_attempt_hook = play_pickup_attempt_hook

import server.config.misc
server.config.misc.make_map = make_map
server.config.misc.decrement_lifepoints = decrement_lifepoints
server.config.misc.calc_effort = calc_effort

import server.config.make_world_helpers
server.config.make_world_helpers.pos_test = pos_test
server.config.make_world_helpers.world_makable = world_makable
server.config.make_world_helpers.make_map = make_map

import server.config.ai
server.config.ai.ai_hook_pickup_test = ai_hook_pickup_test
