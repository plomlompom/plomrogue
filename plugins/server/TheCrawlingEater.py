# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db


def play_drink():
    if action_exists("drink") and world_db["WORLD_ACTIVE"]:
        pos = world_db["Things"][0]["pos"]
        if not (chr(world_db["MAP"][pos]) == "0"
                and world_db["wetmap"][pos] > ord("0")):
            log("NOTHING to drink here.")
            return
        elif world_db["Things"][0]["T_BLADDER"] >= 32:
            log("You're too FULL to drink more.")
            return
        world_db["set_command"]("drink")


def actor_drink(t):
    pos = world_db["Things"][0]["pos"]
    if chr(world_db["MAP"][pos]) == "0" and \
                world_db["wetmap"][pos] > ord("0") and t["T_BLADDER"] < 32:
        log("You DRINK.")
        t["T_BLADDER"] += 1
        world_db["wetmap"][pos] -= 1
        if world_db["wetmap"][pos] == ord("0"):
            world_db["MAP"][pos] = ord("0")


def play_pee():
    if action_exists("pee") and world_db["WORLD_ACTIVE"]:
        if world_db["Things"][0]["T_BLADDER"] < 1:
            log("Nothing to drop from empty bladder.")
            return
        world_db["set_command"]("pee")


def actor_pee(t):
    if t["T_BLADDER"] < 1:
        return
    if t == world_db["Things"][0]:
        log("You LOSE fluid.")
    if not world_db["catch_air"](t):
        return
    t["T_BLADDER"] -= 1
    world_db["wetmap"][t["pos"]] += 1


def play_drop():
    if action_exists("drop") and world_db["WORLD_ACTIVE"]:
        if world_db["Things"][0]["T_BOWEL"] < 1:
            log("Nothing to drop from empty bowel.")
            return
        world_db["set_command"]("drop")


def actor_drop(t):
    if t["T_BOWEL"] < 1:
        return
    if t == world_db["Things"][0]:
        log("You DROP waste.")
    if not world_db["catch_air"](t):
        return
    world_db["MAP"][t["pos"]] += 1
    t["T_BOWEL"] -= 1


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
            if chr(world_db["MAP"][pos]) in "34":
                if t["T_BOWEL"] >= 32:
                    if t == world_db["Things"][0]:
                        log("You're too FULL to eat.")
                    return
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
        height = world_db["MAP"][pos] - ord("0")
        if t["T_BOWEL"] >= 32 or height == 5:
            return
        eaten = False
        if height == 3 and 0 == int(rand.next() % 2):
            t["T_BOWEL"] += height
            eaten = True
        elif height == 4 and 0 == int(rand.next() % 5):
            t["T_BOWEL"] += height
            eaten = True
        log("You EAT.")
        if eaten:
            world_db["MAP"][pos] = ord("0")
            if t["T_BOWEL"] > 32:
                t["T_BOWEL"] = 32



def catch_air(t):
    if (world_db["wetmap"][t["pos"]] - ord("0")) \
            + (world_db["MAP"][t["pos"]] - ord("0")) > 4:
        t["T_LIFEPOINTS"] = 0
        if t == world_db["Things"][0]:
            t["fovmap"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
            log("You SUFFOCATE.")
        return False
    return True
world_db["catch_air"] = catch_air


def make_map():
    from server.make_map import new_pos, is_neighbor
    from server.utils import rand
    world_db["MAP"] = bytearray(b'5' * (world_db["MAP_LENGTH"] ** 2))
    length = world_db["MAP_LENGTH"]
    add_half_width = (not (length % 2)) * int(length / 2)
    world_db["MAP"][int((length ** 2) / 2) + add_half_width] = ord("4")
    while (1):
        y, x, pos = new_pos()
        if "5" == chr(world_db["MAP"][pos]) and is_neighbor((y, x), "4"):
            if y == 0 or y == (length - 1) or x == 0 or x == (length - 1):
                break
            world_db["MAP"][pos] = ord("4")
    n_ground = int((length ** 2) / 16)
    i_ground = 0
    while (i_ground <= n_ground):
        single_allowed = rand.next() % 32
        y, x, pos = new_pos()
        if "4" == chr(world_db["MAP"][pos]) \
                and ((not single_allowed) or is_neighbor((y, x), "0")):
            world_db["MAP"][pos] = ord("0")
            i_ground += 1
    n_water = int((length ** 2) / 64)
    i_water = 0
    while (i_water <= n_water):
        y, x, pos = new_pos()
        if ord("0") == world_db["MAP"][pos] and \
                ord("0") == world_db["wetmap"][pos]:
            world_db["wetmap"][pos] = ord("3")
            i_water += 1


def calc_effort(ta, t):
    from server.utils import mv_yx_in_dir_legal
    if ta["TA_NAME"] == "move":
        move_result = mv_yx_in_dir_legal(chr(t["T_ARGUMENT"]),
                                         t["T_POSY"], t["T_POSX"])
        if 1 == move_result[0]:
            pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
            narrowness = world_db["MAP"][pos] - ord("0")
            return 2 ** narrowness
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
                    if Thing["T_BOWEL"] > 16:
                        if 0 == (rand.next() % (33 - Thing["T_BOWEL"])):
                            action_db["actor_drop"](Thing)
                    if Thing["T_BLADDER"] > 16:
                        if 0 == (rand.next() % (33 - Thing["T_BLADDER"])):
                            action_db["actor_pee"](Thing)
        water = 0
        positions_to_wet = []
        for i in range(world_db["MAP_LENGTH"] ** 2):
            if world_db["MAP"][i] == ord("0") \
                    and world_db["wetmap"][i] < ord("5"):
                positions_to_wet += [i]
        i_positions_to_wet = len(positions_to_wet)
        for pos in range(world_db["MAP_LENGTH"] ** 2):
            if 0 == rand.next() % 5 \
                    and ((world_db["wetmap"][pos] > ord("0")
                          and not world_db["MAP"][pos] == ord("0"))
                         or world_db["wetmap"][pos] > ord("1")):
                world_db["wetmap"][pos] -= 1
                water += 1
                i_positions_to_wet -= 1
            if i_positions_to_wet == 0:
                break
        if water > 0:
            while water > 0:
                select = rand.next() % len(positions_to_wet)
                pos = positions_to_wet[select]
                world_db["wetmap"][pos] += 1
                positions_to_wet.remove(pos)
                water -= 1
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


def save_wetmap():
    length = world_db["MAP_LENGTH"]
    string = ""
    for i in range(length):
        line = world_db["wetmap"][i * length:(i * length) + length].decode()
        string = string + "WETMAP" + " "  + str(i) + " " + line + "\n"
    return string


def wetmapset(str_int, mapline):
    def valid_map_line(str_int, mapline):
        from server.utils import integer_test
        val = integer_test(str_int, 0, 255)
        if None != val:
            if val >= world_db["MAP_LENGTH"]:
                print("Illegal value for map line number.")
            elif len(mapline) != world_db["MAP_LENGTH"]:
                print("Map line length is unequal map width.")
            else:
                return val
        return None
    val = valid_map_line(str_int, mapline)
    if None != val:
        length = world_db["MAP_LENGTH"]
        if not world_db["wetmap"]:
            m = bytearray(b' ' * (length ** 2))
        else:
            m = world_db["wetmap"]
        m[val * length:(val * length) + length] = mapline.encode()
        if not world_db["wetmap"]:
            world_db["wetmap"] = m

def write_wetmap():
    from server.worldstate_write_helpers import write_map
    length = world_db["MAP_LENGTH"]
    visible_wetmap = bytearray(b' ' * (length ** 2))
    for i in range(length ** 2):
        if world_db["Things"][0]["fovmap"][i] == ord('v'):
            visible_wetmap[i] = world_db["wetmap"][i]
    return write_map(visible_wetmap, world_db["MAP_LENGTH"])


from server.config.io import io_db
io_db["worldstate_write_order"] += [["T_BOWEL", "player_int"]]
io_db["worldstate_write_order"] += [["T_BLADDER", "player_int"]]
io_db["worldstate_write_order"] += [[write_wetmap, "func"]]
import server.config.world_data
server.config.world_data.symbols_hide = "345"
server.config.world_data.symbols_passable = "012"
server.config.world_data.thing_defaults["T_BOWEL"] = 0
server.config.world_data.thing_defaults["T_BLADDER"] = 0
world_db["wetmap"] = bytearray(b"0" * world_db["MAP_LENGTH"] ** 2)
io_db["hook_save"] = save_wetmap
import server.config.make_world_helpers
server.config.make_world_helpers.make_map = make_map
from server.config.commands import commands_db
commands_db["THINGS_HERE"] = (2, True, lambda x, y: None)
commands_db["ai"] = (0, False, command_ai)
commands_db["move"] = (1, False, play_move)
commands_db["wait"] = (0, False, play_wait)
commands_db["drop"] = (0, False, play_drop)
commands_db["drink"] = (0, False, play_drink)
commands_db["pee"] = (0, False, play_pee)
commands_db["use"] = (1, False, lambda x: None)
commands_db["pickup"] = (0, False, lambda: None)
commands_db["T_BOWEL"] = (1, False, setter("Thing", "T_BOWEL", 0, 255))
commands_db["T_BLADDER"] = (1, False, setter("Thing", "T_BLADDER", 0, 255))
commands_db["WETMAP"] = (2, False, wetmapset)
from server.actions import actor_wait
import server.config.actions
server.config.actions.action_db = {
    "actor_wait": actor_wait,
    "actor_move": actor_move,
    "actor_drop": actor_drop,
    "actor_drink": actor_drink,
    "actor_pee": actor_pee,
}

strong_write(io_db["file_out"], "PLUGIN TheCrawlingEater\n")
