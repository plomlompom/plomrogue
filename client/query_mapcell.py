from client.config.world_data import world_data
from client.io import send

def query_mapcell():
    string = "THINGS_HERE " + str(world_data["map_center"][0]) + " " \
             + str(world_data["map_center"][1])
    send(string)
    world_data["look"] = ["(polling)"]
