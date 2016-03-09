# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def win_stomach(self):
    winmap = []
    curses.init_pair(80, curses.COLOR_YELLOW, curses.COLOR_RED)
    for i in range(world_data["stomach"]):
        winmap += [("#", curses.color_pair(80))]
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
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_BLUE, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(4, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(5, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
    curses.init_pair(6, curses.COLOR_BLACK, curses.COLOR_BLUE)
    col_unknown = curses.color_pair(1)
    col_mem_obstacle = curses.color_pair(2)
    col_mem = curses.color_pair(2)
    col_stone = curses.color_pair(1)
    col_dirt = curses.color_pair(4)
    col_earth = curses.color_pair(3)
    col_player = curses.color_pair(5)
    col_water = curses.color_pair(6)
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
                elif char == "X" or char == "#":
                    attribute = col_mem_obstacle
                bonus = (" ", attribute)
                winmap += [(char, attribute), bonus]
            else:
                attribute = col_stone
                bonus = " "
                if char == ".":
                    attribute = col_dirt
                elif char == ":":
                    attribute = col_earth
                elif char == "%":
                    attribute = col_earth
                elif char == "#":
                    attribute = col_dirt
                elif char == "~":
                    attribute = col_water
                elif char == "o":
                    attribute = col_player
                    bonus = (char, attribute)
                winmap += [(char, attribute), bonus]
        if y % 2 == 0:
            winmap += "  "
    return offset, winmap_size, winmap

from client.config.world_data import world_data
world_data["stomach"] = 0
from client.config.io import io
io["worldstate_read_order"] += [["stomach", "int"]]
from client.config.windows import windows_config
from client.windows import win_log
windows_config[:] = [
    {"config": [0, -34], "func": win_map, "title": "The Crawling Eater"},
    {"config": [1, 33], "func": win_stomach, "title": "stomach"},
    {"config": [-2, 33], "func": win_log, "title": "log"}
]
from client.window_management import set_windows
set_windows()
from client.commands import command_sender
from client.config.commands import commands
commands["D"] = (command_sender("drop"),)
