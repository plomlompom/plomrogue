# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


import math
import curses

from client.config.world_data import world_data


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
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)
    curses.init_pair(2, curses.COLOR_BLUE, curses.COLOR_BLACK)
    for y in range(world_data["map_size"]):
        for x in range(world_data["map_size"]):
            char = world_data["fov_map"][y * world_data["map_size"] + x]
            if world_data["look_mode"] and y == world_data["map_center"][0] \
                    and x == world_data["map_center"][1]:
                if char == " ":
                    char = \
                        world_data["mem_map"][y * world_data["map_size"] + x]
                winmap += [(char, curses.A_REVERSE), (" ", curses.A_REVERSE)]
                continue
            if char == " ":
                char = world_data["mem_map"][y * world_data["map_size"] + x]
                attribute = curses.color_pair(1) if char == " " \
                    else curses.color_pair(2)
                winmap += [(char, attribute), (" ", attribute)]
            else:
                winmap += char + " "
        if y % 2 == 0:
            winmap += "  "
    return offset, winmap_size, winmap


def win_inventory(self):
    win_size = self.size
    winmap = []
    winmap_size = [0, win_size[1]]
    for line in world_data["inventory"]:
        winmap_size[1] = winmap_size[1] if len(line) <= winmap_size[1] \
            else len(line)
    count = 0
    for line in world_data["inventory"]:
        padding_size = winmap_size[1] - len(line)
        line += (" " * padding_size)
        if count == world_data["inventory_selection"]:
            line_new = []
            for x in range(len(line)):
                line_new += [(line[x], curses.A_REVERSE)]
            line = line_new
        winmap += line
        winmap_size[0] = winmap_size[0] + 1
        count += 1
    offset = [0, 0]
    if world_data["inventory_selection"] > win_size[0]/2:
        if world_data["inventory_selection"] < len(world_data["inventory"]) \
            - win_size[0]/2:
            offset[0] = world_data["inventory_selection"] - int(win_size[0]/2)
        else:
            offset[0] = len(world_data["inventory"]) - win_size[0]
    return offset, winmap_size, winmap


def win_look(self):
    winmap = ""
    winmap_size = [0, 0]
    for line in world_data["look"]:
        winmap_size[1] = winmap_size[1] if len(line) <= winmap_size[1] \
            else len(line)
    for line in world_data["look"]:
        padding_size = winmap_size[1] - len(line)
        winmap += line + (" " * padding_size)
        winmap_size[0] = winmap_size[0] + 1
    offset = [world_data["look_scroll"], 0]
    return offset, winmap_size, winmap


def win_info(self):
    winmap = "T: " + str(world_data["turn"]) \
        + " H: " + str(world_data["lifepoints"]) \
        + " S: " + str(world_data["satiation"])
    winmap_size = [1, len(winmap)]
    offset = [0, 0]
    return offset, winmap_size, winmap


def win_log(self):
    win_size = self.size
    offset = [0, 0]
    winmap = ""
    number_of_lines = 0
    for line in world_data["log"]:
        number_of_lines += math.ceil(len(line) / win_size[1])
        padding_size = win_size[1] - (len(line) % win_size[1])
        winmap += line + (padding_size * " ")
    if number_of_lines < win_size[0]:
        winmap = (" " * win_size[1] * (win_size[0] - number_of_lines)) + winmap
        number_of_lines = win_size[0]
    elif number_of_lines > win_size[0]:
        offset[0] = number_of_lines - win_size[0]
    winmap_size = [number_of_lines, win_size[1]]
    return offset, winmap_size, winmap
