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
        elif world_db["Things"][0]["T_KIDNEY"] >= 32:
            log("You're too FULL to drink more.")
            return
        world_db["set_command"]("drink")


def actor_drink(t):
    pos = world_db["Things"][0]["pos"]
    if chr(world_db["MAP"][pos]) == "0" and \
                world_db["wetmap"][pos] > ord("0") and t["T_KIDNEY"] < 32:
        log("You DRINK.")
        t["T_KIDNEY"] += 1
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
    if not world_db["test_air"](t):
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
    if not world_db["test_air"](t):
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
                if t["T_STOMACH"] >= 32:
                    if t == world_db["Things"][0]:
                        log("You're too FULL to eat.")
                    return
                world_db["Things"][0]["T_ARGUMENT"] = d
                world_db["set_command"]("eat")
                return
            if chr(world_db["MAP"][pos]) in symbols_passable:
                world_db["Things"][0]["T_ARGUMENT"] = d
                world_db["set_command"]("move")
                return
        log("You CAN'T eat your way through there.")


def actor_eat(t):
    from server.utils import mv_yx_in_dir_legal, rand
    from server.config.world_data import symbols_passable
    passable = False
    move_result = mv_yx_in_dir_legal(chr(t["T_ARGUMENT"]),
                                     t["T_POSY"], t["T_POSX"])
    if 1 == move_result[0]:
        pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
        #hitted = [tid for tid in world_db["Things"]
        #          if world_db["Things"][tid] != t
        #          if world_db["Things"][tid]["T_LIFEPOINTS"]
        #          if world_db["Things"][tid]["T_POSY"] == move_result[1]
        #          if world_db["Things"][tid]["T_POSX"] == move_result[2]]
        #if len(hitted):
        #    hit_id = hitted[0]
        #    hitted_tid = world_db["Things"][hit_id]["T_TYPE"]
        #    if t == world_db["Things"][0]:
        #        hitted_name = world_db["ThingTypes"][hitted_tid]["TT_NAME"]
        #        log("You BUMP into " + hitted_name + ".")
        #    elif 0 == hit_id:
        #        hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
        #        log(hitter_name +" BUMPS into you.")
        #    return
        passable = chr(world_db["MAP"][pos]) in symbols_passable
    if passable:
        log("You try to EAT, but fail.")
    else:
        height = world_db["MAP"][pos] - ord("0")
        if t["T_STOMACH"] >= 32 or height == 5:
            return
        t["T_STOMACH"] += 1
        log("You EAT.")
        eaten = (height == 3 and 0 == int(rand.next() % 2)) or \
                (height == 4 and 0 == int(rand.next() % 5))
        if eaten:
            world_db["MAP"][pos] = ord("0")
            if t["T_STOMACH"] > 32:
                t["T_STOMACH"] = 32


def actor_move(t):
    from server.build_fov_map import build_fov_map
    from server.utils import mv_yx_in_dir_legal, rand
    from server.config.world_data import symbols_passable
    passable = False
    move_result = mv_yx_in_dir_legal(chr(t["T_ARGUMENT"]),
                                     t["T_POSY"], t["T_POSX"])
    if 1 == move_result[0]:
        pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
        #hitted = [tid for tid in world_db["Things"]
        #          if world_db["Things"][tid] != t
        #          if world_db["Things"][tid]["T_LIFEPOINTS"]
        #          if world_db["Things"][tid]["T_POSY"] == move_result[1]
        #          if world_db["Things"][tid]["T_POSX"] == move_result[2]]
        #if len(hitted):
        #    hit_id = hitted[0]
        #    hitted_tid = world_db["Things"][hit_id]["T_TYPE"]
        #    if t == world_db["Things"][0]:
        #        hitted_name = world_db["ThingTypes"][hitted_tid]["TT_NAME"]
        #        log("You BUMP into " + hitted_name + ".")
        #    elif 0 == hit_id:
        #        hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
        #        log(hitter_name +" BUMPS into you.")
        #    return
        passable = chr(world_db["MAP"][pos]) in symbols_passable
    if passable:
        t["T_POSY"] = move_result[1]
        t["T_POSX"] = move_result[2]
        t["pos"] = move_result[1] * world_db["MAP_LENGTH"] + move_result[2]
        build_fov_map(t)
    else:
        log("You try to MOVE there, but fail.")


def test_hole(t):
    if world_db["MAP"][t["pos"]] == ord("-"):
        world_db["die"](t, "You FALL in a hole, and die.")
        return False
    return True
world_db["test_hole"] = test_hole


def test_air(t):
    if world_db["terrain_fullness"](t["pos"]) > 5:
        world_db["die"](t, "You SUFFOCATE")
        return False
    return True
world_db["test_air"] = test_air


def die(t, message):
    t["T_LIFEPOINTS"] = 0
    if t == world_db["Things"][0]:
        t["fovmap"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
        t["T_MEMMAP"][t["pos"]] = ord("@")
        log(message)
world_db["die"] = die


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
            t = world_db["Things"][tid]
            if t["T_LIFEPOINTS"]:
                if not (world_db["test_air"](t) and world_db["test_hole"](t)):
                    continue
                if not t["T_COMMAND"]:
                    update_map_memory(t)
                    if 0 == tid:
                        return
                    world_db["ai"](t)
                if t["T_LIFEPOINTS"]:
                    t["T_PROGRESS"] += 1
                    taid = [a for a in world_db["ThingActions"]
                              if a == t["T_COMMAND"]][0]
                    ThingAction = world_db["ThingActions"][taid]
                    effort = world_db["calc_effort"](ThingAction, t)
                    if t["T_PROGRESS"] >= effort:
                        action = action_db["actor_" + ThingAction["TA_NAME"]]
                        action(t)
                        t["T_COMMAND"] = 0
                        t["T_PROGRESS"] = 0
                    if t["T_BOWEL"] > 16:
                        if 0 == (rand.next() % (33 - t["T_BOWEL"])):
                            action_db["actor_drop"](t)
                    if t["T_BLADDER"] > 16:
                        if 0 == (rand.next() % (33 - t["T_BLADDER"])):
                            action_db["actor_pee"](t)
                    if 0 == world_db["TURN"] % 5:
                        t["T_STOMACH"] -= 1
                        t["T_BOWEL"] += 1
                        t["T_KIDNEY"] -= 1
                        t["T_BLADDER"] += 1
                    if t["T_STOMACH"] <= 0:
                        world_db["die"](t, "You DIE of hunger.")
                    elif t["T_KIDNEY"] <= 0:
                        world_db["die"](t, "You DIE of dehydration.")
        positions_to_wet = []
        i_positions_to_wet = len(positions_to_wet)
        for pos in range(world_db["MAP_LENGTH"] ** 2):
            wetness = world_db["wetmap"][pos] - ord("0")
            height = world_db["MAP"][pos] - ord("0")
            if height == 0 and wetness > 0 \
                    and 0 == rand.next() % ((2 ** 13) / (2 ** wetness)):
                world_db["MAP"][pos] = ord("-")
            if ((wetness > 0 and height != 0) or wetness > 1) \
                and 0 == rand.next() % 5:
                world_db["wetmap"][pos] -= 1
                world_db["HUMIDITY"] += 1
                i_positions_to_wet -= 1
        if world_db["HUMIDITY"] > 0:
            for pos in range(world_db["MAP_LENGTH"] ** 2):
                if world_db["MAP"][pos] == ord("0") \
                        and world_db["wetmap"][pos] < ord("5"):
                    positions_to_wet += [pos]
            while world_db["HUMIDITY"] > 0 and len(positions_to_wet) > 0:
                select = rand.next() % len(positions_to_wet)
                pos = positions_to_wet[select]
                world_db["wetmap"][pos] += 1
                positions_to_wet.remove(pos)
                world_db["HUMIDITY"] -= 1
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


def command_ai():
    if world_db["WORLD_ACTIVE"]:
        world_db["ai"](world_db["Things"][0])
        world_db["turn_over"]()


def get_dir_to_target(t, target):

    from server.utils import rand, libpr, c_pointer_to_bytearray
    from server.config.world_data import symbols_passable

    def zero_score_map_where_char_on_memdepthmap(c):
        map = c_pointer_to_bytearray(t["T_MEMDEPTHMAP"])
        if libpr.zero_score_map_where_char_on_memdepthmap(c, map):
            raise RuntimeError("No score map allocated for "
                               "zero_score_map_where_char_on_memdepthmap().")

    def set_map_score(pos, score):
        test = libpr.set_map_score(pos, score)
        if test:
            raise RuntimeError("No score map allocated for set_map_score().")

    def set_movement_cost_map():
        memmap = c_pointer_to_bytearray(t["T_MEMMAP"])
        if libpr.TCE_set_movement_cost_map(memmap):
            raise RuntimeError("No movement cost map allocated for "
                               "set_movement_cost_map().")

    def seeing_thing():
        def exists(gen):
            try:
                next(gen)
            except StopIteration:
                return False
            return True
        mapsize = world_db["MAP_LENGTH"] ** 2
        if target == "food" and t["T_MEMMAP"]:
            return exists(pos for pos in range(mapsize)
                           if ord("2") < t["T_MEMMAP"][pos] < ord("5"))
        elif target == "fluid_certain" and t["fovmap"]:
            return exists(pos for pos in range(mapsize)
                           if t["fovmap"] == ord("v")
                           if world_db["MAP"][pos] == ord("0")
                           if world_db["wetmap"][pos] > ord("0"))
        elif target == "fluid_potential" and t["T_MEMMAP"] and t["fovmap"]:
            return exists(pos for pos in range(mapsize)
                           if t["T_MEMMAP"][pos] == ord("0")
                           if t["fovmap"] != ord("v"))
        elif target == "space" and t["T_MEMMAP"] and t["fovmap"]:
            return exists(pos for pos in range(mapsize)
                          if ord("0") <= t["T_MEMMAP"][pos] <= ord("2")
                          if (t["fovmap"] != ord("v")
                              or world_db["terrain_fullness"](pos) < 5))
        return False

    def init_score_map():
        test = libpr.init_score_map()
        set_movement_cost_map()
        mapsize = world_db["MAP_LENGTH"] ** 2
        if test:
            raise RuntimeError("Malloc error in init_score_map().")
        if target == "food" and t["T_MEMMAP"]:
            [set_map_score(pos, 0) for pos in range(mapsize)
             if ord("2") < t["T_MEMMAP"][pos] < ord("5")]
        elif target == "fluid_certain" and t["fovmap"]:
            [set_map_score(pos, 0) for pos in range(mapsize)
             if t["fovmap"] == ord("v")
             if world_db["MAP"][pos] == ord("0")
             if world_db["wetmap"][pos] > ord("0")]
        elif target == "fluid_potential" and t["T_MEMMAP"] and t["fovmap"]:
            [set_map_score(pos, 0) for pos in range(mapsize)
             if t["T_MEMMAP"][pos] == ord("0")
             if t["fovmap"] != ord("v")]
        elif target == "space" and t["T_MEMMAP"] and t["fovmap"]:
            [set_map_score(pos, 0) for pos in range(mapsize)
             if ord("0") <= t["T_MEMMAP"][pos] <= ord("2")
             if (t["fovmap"] != ord("v")
                 or world_db["terrain_fullness"](pos) < 5)]
        elif target == "search":
            zero_score_map_where_char_on_memdepthmap(mem_depth_c[0])

    def rand_target_dir(neighbors, cmp, dirs):
        candidates = []
        n_candidates = 0
        for i in range(len(dirs)):
            if cmp == neighbors[i]:
                candidates.append(dirs[i])
                n_candidates += 1
        return candidates[rand.next() % n_candidates] if n_candidates else 0

    def get_neighbor_scores(dirs, eye_pos):
        scores = []
        if libpr.ready_neighbor_scores(eye_pos):
            raise RuntimeError("No score map allocated for " +
                               "ready_neighbor_scores.()")
        for i in range(len(dirs)):
            scores.append(libpr.get_neighbor_score(i))
        return scores

    def get_dir_from_neighbors():
        import math
        dir_to_target = False
        dirs = "edcxsw"
        eye_pos = t["pos"]
        neighbors = get_neighbor_scores(dirs, eye_pos)
        minmax_start = 65535 - 1
        minmax_neighbor = minmax_start
        for i in range(len(dirs)):
            if minmax_neighbor > neighbors[i]:
                minmax_neighbor = neighbors[i]
        if minmax_neighbor != minmax_start:
            dir_to_target = rand_target_dir(neighbors, minmax_neighbor, dirs)
        return dir_to_target, minmax_neighbor

    dir_to_target = False
    mem_depth_c = b' '
    run_i = 9 + 1 if "search" == target else 1
    minmax_neighbor = 0
    while run_i and not dir_to_target and \
            ("search" == target or seeing_thing()):
        run_i -= 1
        init_score_map()
        mem_depth_c = b'9' if b' ' == mem_depth_c \
            else bytes([mem_depth_c[0] - 1])
        if libpr.TCE_dijkstra_map_with_movement_cost():
            raise RuntimeError("No score map allocated for dijkstra_map().")
        dir_to_target, minmax_neighbor = get_dir_from_neighbors()
        libpr.free_score_map()
        if dir_to_target and str == type(dir_to_target):
            action = "move"
            from server.utils import mv_yx_in_dir_legal
            move_result = mv_yx_in_dir_legal(dir_to_target, t["T_POSY"],
                                                            t["T_POSX"])
            if 1 != move_result[0]:
                return False, 0
            pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
            if world_db["MAP"][pos] > ord("2"):
                action = "eat"
            t["T_COMMAND"] = [taid for taid in world_db["ThingActions"]
                              if world_db["ThingActions"][taid]["TA_NAME"]
                              == action][0]
            t["T_ARGUMENT"] = ord(dir_to_target)
    return dir_to_target, minmax_neighbor
world_db["get_dir_to_target"] = get_dir_to_target


def terrain_fullness(pos):
    return (world_db["MAP"][pos] - ord("0")) + \
        (world_db["wetmap"][pos] - ord("0"))
world_db["terrain_fullness"] = terrain_fullness


def ai(t):

    if t["T_LIFEPOINTS"] == 0:
        return

    def standing_on_fluid(t):
        if world_db["MAP"][t["pos"]] == ord("0") and \
            world_db["wetmap"][t["pos"]] > ord("0"):
                return True
        else:
            return False

    def thing_action_id(name):
        return [taid for taid in world_db["ThingActions"]
                if world_db["ThingActions"][taid]
                ["TA_NAME"] == name][0]

    t["T_COMMAND"] = thing_action_id("wait")
    needs = {
        "safe_pee": (world_db["terrain_fullness"](t["pos"]) * t["T_BLADDER"]) / 4,
        "safe_drop": (world_db["terrain_fullness"](t["pos"]) * t["T_BOWEL"]) / 4,
        "food": 33 - t["T_STOMACH"],
        "fluid_certain": 33 - t["T_KIDNEY"],
        "fluid_potential": 32 - t["T_KIDNEY"],
        "search": 1,
    }
    from operator import itemgetter
    needs = sorted(needs.items(), key=itemgetter(1,0))
    needs.reverse()
    for need in needs:
        if need[1] > 0:
            if need[0] in {"fluid_certain", "fluid_potential"}:
                if standing_on_fluid(t):
                    t["T_COMMAND"] = thing_action_id("drink")
                    return
                elif t["T_BLADDER"] > 0 and \
                         world_db["MAP"][t["pos"]] == ord("0"):
                    t["T_COMMAND"] = thing_action_id("pee")
                    return
            elif need[0] in {"safe_pee", "safe_drop"}:
                action_name = need[0][len("safe_"):]
                if world_db["terrain_fullness"](t["pos"]) < 4:
                    t["T_COMMAND"] = thing_action_id(action_name)
                    return
                else:
                    test = world_db["get_dir_to_target"](t, "space")
                    if test[0]:
                        if (not test[1] < 5) and \
                                world_db["terrain_fullness"](t["pos"]) < 5:
                            t["T_COMMAND"] = thing_action_id(action_name)
                        return
                    if t["T_STOMACH"] < 32 and \
                            world_db["get_dir_to_target"](t, "food")[0]:
                        return
                continue
            if world_db["get_dir_to_target"](t, need[0])[0]:
                return
            elif t["T_STOMACH"] < 32 and \
                    need[0] in {"fluid_certain", "fluid_potential"} and \
                    world_db["get_dir_to_target"](t, "food")[0]:
                return
world_db["ai"] = ai


from server.config.io import io_db
io_db["worldstate_write_order"] += [["T_STOMACH", "player_int"]]
io_db["worldstate_write_order"] += [["T_KIDNEY", "player_int"]]
io_db["worldstate_write_order"] += [["T_BOWEL", "player_int"]]
io_db["worldstate_write_order"] += [["T_BLADDER", "player_int"]]
io_db["worldstate_write_order"] += [[write_wetmap, "func"]]
import server.config.world_data
server.config.world_data.symbols_hide = "345"
server.config.world_data.symbols_passable = "012-"
server.config.world_data.thing_defaults["T_STOMACH"] = 16
server.config.world_data.thing_defaults["T_BOWEL"] = 0
server.config.world_data.thing_defaults["T_KIDNEY"] = 16
server.config.world_data.thing_defaults["T_BLADDER"] = 0
world_db["wetmap"] = bytearray(b"0" * world_db["MAP_LENGTH"] ** 2)
if not "HUMIDITY" in world_db:
    world_db["HUMIDITY"] = 0
io_db["hook_save"] = save_wetmap
import server.config.make_world_helpers
server.config.make_world_helpers.make_map = make_map
from server.config.commands import commands_db
commands_db["THINGS_HERE"] = (2, True, lambda x, y: None)
commands_db["ai"] = (0, False, command_ai)
commands_db["move"] = (1, False, play_move)
commands_db["eat"] = (1, False, play_move)
commands_db["wait"] = (0, False, play_wait)
commands_db["drop"] = (0, False, play_drop)
commands_db["drink"] = (0, False, play_drink)
commands_db["pee"] = (0, False, play_pee)
commands_db["use"] = (1, False, lambda x: None)
commands_db["pickup"] = (0, False, lambda: None)
commands_db["HUMIDITY"] = (1, False, setter(None, "HUMIDITY", 0, 65535))
commands_db["T_STOMACH"] = (1, False, setter("Thing", "T_STOMACH", 0, 255))
commands_db["T_KIDNEY"] = (1, False, setter("Thing", "T_KIDNEY", 0, 255))
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
    "actor_eat": actor_eat,
}

strong_write(io_db["file_out"], "PLUGIN TheCrawlingEater\n")
