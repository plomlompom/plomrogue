from client.config.io import io
from client.config.world_data import world_data
world_data["log"][-2] = "STATS OVERVIEW: " \
"T: turn, H: health, S: satiation, G: god's favor."
world_data["GOD_FAVOR"] = 0
io["worldstate_read_order"] += [["GOD_FAVOR", "int"]]
world_data["metamap_A"] = ""
world_data["metamap_B"] = ""
io["worldstate_read_order"] += [["metamap_A", "map"]]
io["worldstate_read_order"] += [["metamap_B", "map"]]

def win_info(self):
    winmap = "T: " + str(world_data["turn"]) \
        + " H: " + str(world_data["lifepoints"]) \
        + " S: " + str(world_data["satiation"]) \
        + " G: " + str(world_data["GOD_FAVOR"])
    winmap_size = [1, len(winmap)]
    offset = [0, 0]
    return offset, winmap_size, winmap

def win_map(self):
    win_size = self.size
    offset = [0, 0]
    for i in range(2):
        if world_data["map_center"][i] * (i + 1) > win_size[i] / 2 and \
                win_size[i] < world_data["map_size"] * (i + 1):
            if world_data["map_center"][i] * (i + 1) \
                < world_data["map_size"] * (i + 1) - win_size[i] / 2:
                offset[i] = world_data["map_center"][i] * (i + 1) \
                    - int(win_size[i] / 2)
                if i == 1:
                    offset[1] = offset[1] + world_data["map_center"][0] % 2
            else:
                offset[i] = world_data["map_size"] * (i + 1) - win_size[i] + i
    winmap_size = [world_data["map_size"], world_data["map_size"] * 2 + 1]
    winmap = []
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_WHITE)
    curses.init_pair(2, curses.COLOR_BLUE, curses.COLOR_WHITE)
    curses.init_pair(3, curses.COLOR_RED, curses.COLOR_WHITE)
    curses.init_pair(4, curses.COLOR_WHITE, curses.COLOR_BLUE)
    curses.init_pair(5, curses.COLOR_BLUE, curses.COLOR_RED)
    curses.init_pair(6, curses.COLOR_BLACK, curses.COLOR_RED)
    curses.init_pair(7, curses.COLOR_BLUE, curses.COLOR_GREEN)
    curses.init_pair(8, curses.COLOR_BLUE, curses.COLOR_YELLOW)
    curses.init_pair(9, curses.COLOR_BLACK, curses.COLOR_MAGENTA)
    curses.init_pair(10, curses.COLOR_BLACK, curses.COLOR_CYAN)
    curses.init_pair(11, curses.COLOR_WHITE, curses.COLOR_BLACK)
    curses.init_pair(12, curses.COLOR_BLUE, curses.COLOR_BLACK)
    curses.init_pair(13, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(14, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(15, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(16, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(17, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
    curses.init_pair(18, curses.COLOR_WHITE, curses.COLOR_MAGENTA)
    col_unknown = curses.color_pair(1)
    col_mem = curses.color_pair(4)
    col_mem_obstacle = curses.color_pair(2)
    col_mem_altar = curses.color_pair(18)
    col_plant = curses.color_pair(11)
    col_altar = curses.color_pair(10)
    col_tool = curses.color_pair(16)
    col_corpse = curses.color_pair(17)
    col_unkraut = curses.color_pair(14)
    col_animal = curses.color_pair(6)
    col_player = curses.color_pair(3)
    col_ground = curses.color_pair(15)
    col_obstacle = curses.color_pair(9)
    col_health_good = curses.color_pair(7)
    col_health_middle = curses.color_pair(8)
    col_health_bad = curses.color_pair(5)
    col_lumber = curses.color_pair(13)
    col_water = curses.color_pair(12)
    col_stack = curses.color_pair(11)
    for y in range(world_data["map_size"]):
        for x in range(world_data["map_size"]):
            pos = y * world_data["map_size"] + x
            char = world_data["fov_map"][pos]
            if world_data["look_mode"] and y == world_data["map_center"][0] \
                    and x == world_data["map_center"][1]:
                if char == " ":
                    char = world_data["mem_map"][pos]
                winmap += [(char, curses.A_REVERSE), ("?", curses.A_REVERSE)]
                continue
            if char == " ":
                char = world_data["mem_map"][pos]
                attribute = col_mem
                if char == " ":
                    attribute = col_unknown
                elif char == "X" or char == "|":
                    attribute = col_mem_obstacle
                elif char == "_":
                    attribute = col_mem_altar
                bonus = (" ", attribute)
                if len(world_data["metamap_A"]) > 0 and \
                        world_data["metamap_A"][pos] == "2":
                    bonus = ("+", col_mem)
                winmap += [(char, attribute), bonus]
            else:
                attribute = col_water
                if char == "." or char == ":":
                    attribute = col_ground
                elif char == "@":
                    attribute = col_player
                elif char == "," or char == "d" or char == "B":
                    attribute = col_animal
                elif char == "#":
                    attribute = col_unkraut
                elif char == "$" or char == "%" or char == ";" or char == "&":
                    attribute = col_corpse
                elif char == "-" or char == "/" or char == "]" or char == "[":
                    attribute = col_tool
                elif char == "X" or char == "|":
                    attribute = col_obstacle
                elif char == "_":
                    attribute = col_altar
                elif char == "(" or char == "*":
                    attribute = col_plant
                elif char == "=":
                    attribute = col_lumber
                bonus = (" ", attribute)
                if len(world_data["metamap_A"]) > 0:
                    if world_data["metamap_A"][pos] == "2":
                        bonus = ("+", col_stack)
                    elif not world_data["metamap_A"][pos] in "01":
                        c = world_data["metamap_B"][pos]
                        if world_data["metamap_A"][pos] == "a":
                            bonus = (c, col_health_bad)
                        elif world_data["metamap_A"][pos] == "b":
                            bonus = (c, col_health_middle)
                        elif world_data["metamap_A"][pos] == "c":
                            bonus = (c, col_health_good)
                winmap += [(char, attribute), bonus]
        if y % 2 == 0:
            winmap += "  "
    return offset, winmap_size, winmap

from client.config.windows import windows_config
from client.windows import win_log, win_inventory, win_look
windows_config[:] = [
    {"config": [1, 33], "func": win_info, "title": "Stats"},
    {"config": [-7, 33], "func": win_log, "title": "Log"},
    {"config": [4, 16], "func": win_inventory, "title": "Inventory"},
    {"config": [4, 16], "func": win_look, "title": "Things here"},
    {"config": [0, -34], "func": win_map, "title": "PLEASE THE ISLAND GOD"}
]
from client.window_management import set_windows
set_windows()
