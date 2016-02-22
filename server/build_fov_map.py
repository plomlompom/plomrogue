# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def build_fov_map(t):
    """Build Thing's FOV map."""
    from server.config.world_data import world_db
    from server.utils import libpr, c_pointer_to_bytearray
    t["fovmap"] = bytearray(b'v' * (world_db["MAP_LENGTH"] ** 2))
    fovmap = c_pointer_to_bytearray(t["fovmap"])
    map = c_pointer_to_bytearray(world_db["MAP"])
    if libpr.build_fov_map(t["T_POSY"], t["T_POSX"], fovmap, map):
        raise RuntimeError("Malloc error in build_fov_Map().")
