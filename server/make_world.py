# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db, symbols_passable
from server.config.make_world_helpers import make_map, world_makable, pos_test
from server.config.io import io_db
from server.utils import rand, libpr, id_setter
from server.new_thing import new_Thing
from server.io import strong_write
from server.update_map_memory import update_map_memory


def make_world(seed):
    """(Re-)build game world, i.e. map, things, to a new turn 1 from seed.

    Seed rand with seed. Do more only with a "wait" ThingAction and
    world["PLAYER_TYPE"] matching ThingType of TT_START_NUMBER > 0. Then,
    world_db["Things"] emptied, call make_map() and set
    world_db["WORLD_ACTIVE"], world_db["TURN"] to 1. Build new Things
    according to ThingTypes' TT_START_NUMBERS, with Thing of ID 0 to ThingType
    of ID = world["PLAYER_TYPE"]. Place Things randomly, and actors not on each
    other. Init player's memory map. Write "NEW_WORLD" line to out file.
    """
    def free_pos(type):
        i = 0
        while 1:
            err = "Space to put thing on too hard to find. Map too small?"
            while 1:
                y = rand.next() % world_db["MAP_LENGTH"]
                x = rand.next() % world_db["MAP_LENGTH"]
                if chr(world_db["MAP"][y * world_db["MAP_LENGTH"] + x]) in \
                    symbols_passable and pos_test(type, y, x):
                    break
                i += 1
                if i == 65535:
                    raise SystemExit(err)
            # Replica of C code, wrongly ignores animatedness of new Thing.
            pos_clear = (0 == len([id for id in world_db["Things"]
                                   if world_db["Things"][id]["T_LIFEPOINTS"]
                                   if world_db["Things"][id]["T_POSY"] == y
                                   if world_db["Things"][id]["T_POSX"] == x]))
            if pos_clear:
                break
        return (y, x)

    playertype = world_makable()
    if playertype < 0:
        return
    rand.seed = seed
    libpr.set_maplength(world_db["MAP_LENGTH"])
    world_db["Things"] = {}
    make_map()
    world_db["WORLD_ACTIVE"] = 1
    world_db["TURN"] = 1
    for i in range(world_db["ThingTypes"][playertype]["TT_START_NUMBER"]):
        id = id_setter(-1, "Things")
        world_db["Things"][id] = new_Thing(playertype, free_pos(playertype))
    if not world_db["Things"][0]["fovmap"]:
        empty_fovmap = bytearray(b" " * world_db["MAP_LENGTH"] ** 2)
        world_db["Things"][0]["fovmap"] = empty_fovmap
    update_map_memory(world_db["Things"][0])
    for type in world_db["ThingTypes"]:
        for i in range(world_db["ThingTypes"][type]["TT_START_NUMBER"]):
            if type != playertype:
                id = id_setter(-1, "Things")
                world_db["Things"][id] = new_Thing(type, free_pos(type))
    strong_write(io_db["file_out"], "NEW_WORLD\n")
