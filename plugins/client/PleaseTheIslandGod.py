from client.config.io import io
from client.config.world_data import world_data
world_data["GOD_FAVOR"] = 0
io["worldstate_read_order"] += [["GOD_FAVOR", "int"]]

def win_info(self):
    winmap = "T: " + str(world_data["turn"]) \
        + " H: " + str(world_data["lifepoints"]) \
        + " S: " + str(world_data["satiation"]) \
        + " G: " + str(world_data["GOD_FAVOR"])
    winmap_size = [1, len(winmap)]
    offset = [0, 0]
    return offset, winmap_size, winmap

from client.config.windows import windows_config
from client.windows import win_log, win_inventory, win_look, win_map
windows_config[:] = [
    {"config": [1, 33], "func": win_info, "title": "Info"},
    {"config": [-7, 33], "func": win_log, "title": "Log"},
    {"config": [4, 16], "func": win_inventory, "title": "Inventory"},
    {"config": [4, 16], "func": win_look, "title": "Things here"},
    {"config": [0, -34], "func": win_map, "title": "Map"}
]
from client.window_management import set_windows
set_windows()
