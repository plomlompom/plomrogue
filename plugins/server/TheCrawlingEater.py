# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db


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
    n_trees = int((length ** 2) / 16)
    i_trees = 0
    while (i_trees <= n_trees):
        single_allowed = rand.next() % 32
        y, x, pos = new_pos()
        if "#" == chr(world_db["MAP"][pos]) \
                and ((not single_allowed) or is_neighbor((y, x), ".")):
            world_db["MAP"][pos] = ord(".")
            i_trees += 1


def actor_move_attempts_hook(t, move_result, pos):
    from server.utils import rand
    if ord("#") == world_db["MAP"][pos] and 0 == int(rand.next() % 5):
        world_db["MAP"][pos] = ord(".")
        return True
    return False


def turn_over():
    from server.ai import ai
    from server.config.actions import action_db
    from server.config.misc import calc_effort
    from server.update_map_memory import update_map_memory
    from server.io import try_worldstate_update
    from server.config.io import io_db
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
                    effort = calc_effort(ThingAction, Thing)
                    if Thing["T_PROGRESS"] == effort:
                        action = action_db["actor_" + ThingAction["TA_NAME"]]
                        action(Thing)
                        Thing["T_COMMAND"] = 0
                        Thing["T_PROGRESS"] = 0
        world_db["TURN"] += 1
        io_db["worldstate_updateable"] = True
        try_worldstate_update()
world_db["turn_over"] = turn_over


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
            if ord("#") == world_db["MAP"][pos]:
                log("You EAT.")
                world_db["Things"][0]["T_ARGUMENT"] = d
                world_db["set_command"]("move")
                return
            if chr(world_db["MAP"][pos]) in symbols_passable:
                world_db["Things"][0]["T_ARGUMENT"] = d
                world_db["set_command"]("move")
                return
        log("You CAN'T eat your way through there.")


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


import server.config.actions
server.config.actions.actor_move_attempts_hook = actor_move_attempts_hook
import server.config.world_data
server.config.world_data.symbols_hide += "#"
import server.config.make_world_helpers
server.config.make_world_helpers.make_map = make_map
from server.config.commands import commands_db
commands_db["ai"] = (0, False, command_ai)
commands_db["move"] = (1, False, play_move)
commands_db["wait"] = (0, False, play_wait)
commands_db["drop"] = (1, False, lambda x: None)
commands_db["use"] = (1, False, lambda x: None)
commands_db["pickup"] = (0, False, lambda: None)

strong_write(io_db["file_out"], "PLUGIN TheCrawlingEater\n")
