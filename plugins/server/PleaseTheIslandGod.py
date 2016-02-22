from server.io import log
from server.config.world_data import world_db

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


def actor_move(t):
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
            log("BAR " + str(t["T_LIFEPOINTS"]))
            return sadness 
        return 0 
    from server.world import build_fov_map
    from server.utils import mv_yx_in_dir_legal
    from server.config.world_data import directions_db
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
            test = decrement_lifepoints(world_db["Things"][hit_id])
            if test and t == world_db["Things"][0]:
                world_db["GOD_FAVOR"] -= test 
            return
        passable = "." == chr(world_db["MAP"][pos]) or \
                   ":" == chr(world_db["MAP"][pos])
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

from server.io import strong_write
strong_write(io_db["file_out"], "PLUGIN PleaseTheIslandGod\n")

if not "GOD_FAVOR"  in world_db:
    world_db["GOD_FAVOR"] = 0
from server.config.io import io_db
io_db["worldstate_write_order"] += [["GOD_FAVOR", "world_int"]]

import server.config.world_data
server.config.world_data.symbols_passable += ":"

from server.config.world_data import thing_defaults
thing_defaults["T_PLAYERDROP"] = 0

import server.config.actions
server.config.actions.action_db["actor_move"] = actor_move
server.config.actions.action_db["actor_pickup"] = actor_pickup
server.config.actions.action_db["actor_drop"] = actor_drop
server.config.actions.ai_func = ai

from server.config.commands import commands_db
commands_db["TT_ID"] = (1, False, command_ttid)
commands_db["GOD_FAVOR"] = (1, False, setter(None, "GOD_FAVOR", -32768, 32767))
commands_db["TT_STORAGE"] = (1, False, setter("ThingType", "TT_STORAGE", 0, 255))
commands_db["T_PLAYERDROP"] = (1, False, setter("Thing", "T_PLAYERDROP", 0, 1))
