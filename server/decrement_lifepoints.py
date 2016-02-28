# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def decrement_lifepoints(t):
    """Decrement t's lifepoints by 1, and if to zero, corpse it.

    If t is the player avatar, only blank its fovmap, so that the client may
    still display memory data. On non-player things, erase fovmap and memory.
    Dying actors drop all their things.
    """
    from server.config.world_data import world_db
    from server.io import log
    t["T_LIFEPOINTS"] -= 1
    if 0 == t["T_LIFEPOINTS"]:
        live_tid = t["T_TYPE"]
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
        return world_db["ThingTypes"][live_tid]["TT_LIFEPOINTS"]
    return 0
