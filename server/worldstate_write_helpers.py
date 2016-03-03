# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db


def write_inventory():
    inventory = ""
    if [] == world_db["Things"][0]["T_CARRIES"]:
        inventory = "(none)\n"
    else:
        for id in world_db["Things"][0]["T_CARRIES"]:
            type_id = world_db["Things"][id]["T_TYPE"]
            name = world_db["ThingTypes"][type_id]["TT_NAME"]
            inventory = inventory + name + "\n"
    inventory += "%\n"
    return inventory

def write_map(map, length):
    string = ""
    for i in range(length):
        line = map[i * length:(i * length) + length].decode()
        string = string + line + "\n"
    return string

def write_fov_map():
    length = world_db["MAP_LENGTH"]
    fov = bytearray(b' ' * (length ** 2))
    ord_v = ord("v")
    for pos in [pos for pos in range(length ** 2)
                    if ord_v == world_db["Things"][0]["fovmap"][pos]]:
        fov[pos] = world_db["MAP"][pos]
    for id in [id for tid in reversed(sorted(list(world_db["ThingTypes"])))
                  for id in world_db["Things"]
                  if not world_db["Things"][id]["carried"]
                  if world_db["Things"][id]["T_TYPE"] == tid
                  if world_db["Things"][0]["fovmap"][
                       world_db["Things"][id]["pos"]] == ord_v]:
        type = world_db["Things"][id]["T_TYPE"]
        c = ord(world_db["ThingTypes"][type]["TT_SYMBOL"])
        fov[world_db["Things"][id]["pos"]] = c
    return write_map(fov, length)

def write_mem_map():
    length = world_db["MAP_LENGTH"]
    mem = world_db["Things"][0]["T_MEMMAP"][:]
    for mt in [mt for tid in reversed(sorted(list(world_db["ThingTypes"])))
                  for mt in world_db["Things"][0]["T_MEMTHING"]
                  if mt[0] == tid]:
         c = world_db["ThingTypes"][mt[0]]["TT_SYMBOL"]
         mem[(mt[1] * length) + mt[2]] = ord(c)
    return write_map(mem, length)
