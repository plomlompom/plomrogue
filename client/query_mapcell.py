# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from client.config.world_data import world_data
from client.io import send


def query_mapcell():
    string = "THINGS_HERE " + str(world_data["map_center"][0]) + " " \
             + str(world_data["map_center"][1])
    send(string)
    world_data["look"] = ["(polling)"]
    world_data["look_scroll"] = 0
