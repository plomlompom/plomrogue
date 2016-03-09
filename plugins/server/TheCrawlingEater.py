# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db


def play_drop():
    if action_exists("drop") and world_db["WORLD_ACTIVE"]:
        if world_db["Things"][0]["T_STOMACH"] < 1:
            log("Nothing to drop from empty stomach.")
            return
        world_db["set_command"]("drop")


def actor_drop(t):
    if t["T_STOMACH"] < 1:
        return
    if t == world_db["Things"][0]:
        log("You DEFECATE.")
    terrain = world_db["MAP"][t["pos"]]
    t["T_STOMACH"] -= 1
    if chr(terrain) == "_":
        world_db["MAP"][t["pos"]] = ord(".")
    elif chr(terrain) == ".":
        world_db["MAP"][t["pos"]] = ord(":")
    elif chr(terrain) == ":":
        world_db["MAP"][t["pos"]] = ord("%")
    elif chr(terrain) == "%":
        world_db["MAP"][t["pos"]] = ord("#")
    elif chr(terrain) == "#":
        world_db["MAP"][t["pos"]] = ord("X")
    elif chr(terrain) == "X":
        t["T_LIFEPOINTS"] = 0
        if t == world_db["Things"][0]:
            t["fovmap"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
            log("You SUFFOCATE.")


def play_move(str_arg):
    """Try "move" as player's T_COMMAND, str_arg as T_ARGUMENT / direction."""
    if action_exists("move") and world_db["WORLD_ACTIVE"]:
        from server.config.world_data import directions_db, symbols_passable
        t = world_db["Things"][0]
        if not str_arg in directions_db:
            print("Illegal move direction string.")
            return
        d = ord(directions_db[str_arg])
        from server.utils import mv_yx_in_dir_legal
        move_result = mv_yx_in_dir_legal(chr(d), t["T_POSY"], t["T_POSX"])
        if 1 == move_result[0]:
            pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
            if ord("%") == world_db["MAP"][pos] or \
                    ord("#") == world_db["MAP"][pos]:
                world_db["Things"][0]["T_ARGUMENT"] = d
                world_db["set_command"]("move")
                return
            if chr(world_db["MAP"][pos]) in symbols_passable:
                world_db["Things"][0]["T_ARGUMENT"] = d
                world_db["set_command"]("move")
                return
        log("You CAN'T eat your way through there.")


def actor_move(t):
    from server.build_fov_map import build_fov_map
    from server.utils import mv_yx_in_dir_legal, rand
    from server.config.world_data import directions_db, symbols_passable
    passable = False
    move_result = mv_yx_in_dir_legal(chr(t["T_ARGUMENT"]),
                                     t["T_POSY"], t["T_POSX"])
    if 1 == move_result[0]:
        pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
        hitted = [tid for tid in world_db["Things"]
                  if world_db["Things"][tid] != t
                  if world_db["Things"][tid]["T_LIFEPOINTS"]
                  if world_db["Things"][tid]["T_POSY"] == move_result[1]
                  if world_db["Things"][tid]["T_POSX"] == move_result[2]]
        if len(hitted):
            hit_id = hitted[0]
            hitted_tid = world_db["Things"][hit_id]["T_TYPE"]
            if t == world_db["Things"][0]:
                hitted_name = world_db["ThingTypes"][hitted_tid]["TT_NAME"]
                log("You BUMP into " + hitted_name + ".")
            elif 0 == hit_id:
                hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
                log(hitter_name +" BUMPS into you.")
            return
        passable = chr(world_db["MAP"][pos]) in symbols_passable
    direction = [direction for direction in directions_db
                 if directions_db[direction] == chr(t["T_ARGUMENT"])][0]
    if passable:
        t["T_POSY"] = move_result[1]
        t["T_POSX"] = move_result[2]
        t["pos"] = move_result[1] * world_db["MAP_LENGTH"] + move_result[2]
        build_fov_map(t)
    else:
        if t["T_STOMACH"] >= 32:
            if t == world_db["Things"][0]:
                log("You're too FULL to eat.")
        elif ord("%") == world_db["MAP"][pos] and 0 == int(rand.next() % 2):
            log("You EAT.")
            world_db["MAP"][pos] = ord("_")
            t["T_STOMACH"] += 3
        elif ord("#") == world_db["MAP"][pos] and 0 == int(rand.next() % 5):
            log("You EAT.")
            world_db["MAP"][pos] = ord("_")
            t["T_STOMACH"] += 4
        if t["T_STOMACH"] > 32:
            t["T_STOMACH"] = 32


def make_map():
    from server.make_map import new_pos, is_neighbor
    from server.utils import rand
    world_db["MAP"] = bytearray(b'X' * (world_db["MAP_LENGTH"] ** 2))
    length = world_db["MAP_LENGTH"]
    add_half_width = (not (length % 2)) * int(length / 2)
    world_db["MAP"][int((length ** 2) / 2) + add_half_width] = ord("#")
    while (1):
        y, x, pos = new_pos()
        if "X" == chr(world_db["MAP"][pos]) and is_neighbor((y, x), "#"):
            if y == 0 or y == (length - 1) or x == 0 or x == (length - 1):
                break
            world_db["MAP"][pos] = ord("#")
    n_ground = int((length ** 2) / 16)
    i_ground = 0
    while (i_ground <= n_ground):
        single_allowed = rand.next() % 32
        y, x, pos = new_pos()
        if "#" == chr(world_db["MAP"][pos]) \
                and ((not single_allowed) or is_neighbor((y, x), "_")):
            world_db["MAP"][pos] = ord("_")
            i_ground += 1
    n_water = int((length ** 2) / 64)
    i_water = 0
    while (i_water <= n_water):
        single_allowed = rand.next() % 32
        y, x, pos = new_pos()
        if "_" == chr(world_db["MAP"][pos]) \
                and ((not single_allowed) or is_neighbor((y, x), "~")):
            world_db["MAP"][pos] = ord("~")
            i_water += 1


def calc_effort(ta, t):
    from server.utils import mv_yx_in_dir_legal
    if ta["TA_NAME"] == "move":
        move_result = mv_yx_in_dir_legal(chr(t["T_ARGUMENT"]),
                                         t["T_POSY"], t["T_POSX"])
        if 1 == move_result[0]:
            pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
            terrain = chr(world_db["MAP"][pos])
            if terrain == ".":
                return 2
            elif terrain == ":":
                return 4
    return 1
world_db["calc_effort"] = calc_effort


def turn_over():
    from server.ai import ai
    from server.config.actions import action_db
    from server.update_map_memory import update_map_memory
    from server.io import try_worldstate_update
    from server.config.io import io_db
    from server.utils import rand
    while world_db["Things"][0]["T_LIFEPOINTS"]:
        for tid in [tid for tid in world_db["Things"]]:
            if not tid in world_db["Things"]:
                continue
            Thing = world_db["Things"][tid]
            if Thing["T_LIFEPOINTS"]:
                if not Thing["T_COMMAND"]:
                    update_map_memory(Thing)
                    if 0 == tid:
                        return
                    ai(Thing)
                if Thing["T_LIFEPOINTS"]:
                    Thing["T_PROGRESS"] += 1
                    taid = [a for a in world_db["ThingActions"]
                              if a == Thing["T_COMMAND"]][0]
                    ThingAction = world_db["ThingActions"][taid]
                    effort = world_db["calc_effort"](ThingAction, Thing)
                    if Thing["T_PROGRESS"] >= effort:
                        action = action_db["actor_" + ThingAction["TA_NAME"]]
                        action(Thing)
                        Thing["T_COMMAND"] = 0
                        Thing["T_PROGRESS"] = 0
                    if Thing["T_STOMACH"] > 16:
                        if 0 == (rand.next() % (33 - Thing["T_STOMACH"])):
                            action_db["actor_drop"](Thing)
        world_db["TURN"] += 1
        io_db["worldstate_updateable"] = True
        try_worldstate_update()
world_db["turn_over"] = turn_over


def command_ai():
    """Call ai() on player Thing, then turn_over()."""
    from server.ai import ai
    if world_db["WORLD_ACTIVE"]:
        ai(world_db["Things"][0])
        world_db["turn_over"]()


def set_command(action):
    """Set player's T_COMMAND, then call turn_over()."""
    tid = [x for x in world_db["ThingActions"]
           if world_db["ThingActions"][x]["TA_NAME"] == action][0]
    world_db["Things"][0]["T_COMMAND"] = tid
    world_db["turn_over"]()
world_db["set_command"] = set_command


def play_wait():
    """Try "wait" as player's T_COMMAND."""
    if world_db["WORLD_ACTIVE"]:
        world_db["set_command"]("wait")


from server.config.io import io_db
io_db["worldstate_write_order"] += [["T_STOMACH", "player_int"]]
import server.config.world_data
server.config.world_data.symbols_hide = "%#X"
server.config.world_data.symbols_passable = "_.:"
server.config.world_data.thing_defaults["T_STOMACH"] = 0
import server.config.make_world_helpers
server.config.make_world_helpers.make_map = make_map
from server.config.commands import commands_db
commands_db["THINGS_HERE"] = (2, True, lambda x, y: None)
commands_db["ai"] = (0, False, command_ai)
commands_db["move"] = (1, False, play_move)
commands_db["wait"] = (0, False, play_wait)
commands_db["drop"] = (0, False, play_drop)
commands_db["use"] = (1, False, lambda x: None)
commands_db["pickup"] = (0, False, lambda: None)
commands_db["T_STOMACH"] = (1, False, setter("Thing", "T_STOMACH", 0, 255))
from server.actions import actor_wait
import server.config.actions
server.config.actions.action_db = {
    "actor_wait": actor_wait,
    "actor_move": actor_move,
    "actor_drop": actor_drop
}

strong_write(io_db["file_out"], "PLUGIN TheCrawlingEater\n")
