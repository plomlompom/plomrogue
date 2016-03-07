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


def play_move_attempt_hook(t, direction, pos):
    if ord("#") == world_db["MAP"][pos]:
        log("You EAT.")
        world_db["Things"][0]["T_ARGUMENT"] = direction
        set_command("move")
        return True
    return False


import server.config.actions
server.config.actions.actor_move_attempts_hook = actor_move_attempts_hook
import server.config.commands
server.config.commands.play_move_attempt_hook = play_move_attempt_hook
import server.config.world_data
server.config.world_data.symbols_hide += "#"
import server.config.make_world_helpers
server.config.make_world_helpers.make_map = make_map

strong_write(io_db["file_out"], "PLUGIN TheCrawlingEater\n")
