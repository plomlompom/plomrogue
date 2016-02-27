# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db
from server.utils import rand


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

def new_pos():
    length = world_db["MAP_LENGTH"]
    y = rand.next() % length
    x = rand.next() % length
    return y, x, (y * length) + x

def make_map():
    """(Re-)make island map.

    Let "~" represent water, "." land, "X" trees: Build island shape randomly,
    start with one land cell in the middle, then go into cycle of repeatedly
    selecting a random sea cell and transforming it into land if it is neighbor
    to land. The cycle ends when a land cell is due to be created at the map's
    border. Then put some trees on the map (TODO: more precise algorithm desc).
    """
    world_db["MAP"] = bytearray(b'~' * (world_db["MAP_LENGTH"] ** 2))
    length = world_db["MAP_LENGTH"]
    add_half_width = (not (length % 2)) * int(length / 2)
    world_db["MAP"][int((length ** 2) / 2) + add_half_width] = ord(".")
    while (1):
        y, x, pos = new_pos()
        if "~" == chr(world_db["MAP"][pos]) and is_neighbor((y, x), "."):
            if y == 0 or y == (length - 1) or x == 0 or x == (length - 1):
                break
            world_db["MAP"][pos] = ord(".")
    n_trees = int((length ** 2) / 16)
    i_trees = 0
    while (i_trees <= n_trees):
        single_allowed = rand.next() % 32
        y, x, pos = new_pos()
        if "." == chr(world_db["MAP"][pos]) \
                and ((not single_allowed) or is_neighbor((y, x), "X")):
            world_db["MAP"][pos] = ord("X")
            i_trees += 1
