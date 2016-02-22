# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def new_Thing(_type, pos=(0, 0)):
    """Return Thing of type T_TYPE, with fovmap if alive and world active."""
    from server.config.world_data import thing_defaults, world_db
    from server.build_fov_map import build_fov_map
    thing = {}
    for key in thing_defaults:
        thing[key] = thing_defaults[key]
        if type(thing[key]) == list:
            thing[key] = thing[key][:]
    thing["T_LIFEPOINTS"] = world_db["ThingTypes"][_type]["TT_LIFEPOINTS"]
    thing["T_TYPE"] = _type
    thing["T_POSY"] = pos[0]
    thing["T_POSX"] = pos[1]
    if world_db["WORLD_ACTIVE"] and thing["T_LIFEPOINTS"]:
        build_fov_map(thing)
    return thing
