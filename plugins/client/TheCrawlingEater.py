# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


curses.init_pair(76, curses.COLOR_BLACK, curses.COLOR_WHITE)
curses.init_pair(77, curses.COLOR_WHITE, curses.COLOR_GREEN)
curses.init_pair(78, curses.COLOR_BLACK, curses.COLOR_RED)
curses.init_pair(79, curses.COLOR_WHITE, curses.COLOR_BLUE)
curses.init_pair(80, curses.COLOR_BLACK, curses.COLOR_YELLOW)


def win_bar_maker(color_number, symbol, title):
    def win_bar(self):
        winmap = []
        for i in range(world_data[title]):
            if i == 32:
                break
            winmap += [(symbol, curses.color_pair(color_number))]
        winmap_size = [1, len(winmap)]
        offset = [0, 0]
        return offset, winmap_size, winmap
    return win_bar


def win_map(self):
    charmap = {
        "0": "_",
        "1": ".",
        "2": ":",
        "3": "%",
        "4": "#",
        "5": "X",
        "*": "O",
        "&": "0",
    }
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
    #curses.init_pair(2, curses.COLOR_BLUE, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(4, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(5, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(6, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(7, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
    curses.init_pair(8, curses.COLOR_BLACK, curses.COLOR_WHITE)
    curses.init_pair(9, curses.COLOR_BLACK, curses.COLOR_BLUE)
    curses.init_pair(10, curses.COLOR_BLACK, curses.COLOR_CYAN)
    curses.init_pair(11, curses.COLOR_BLACK, curses.COLOR_GREEN)
    curses.init_pair(12, curses.COLOR_BLACK, curses.COLOR_YELLOW)
    curses.init_pair(13, curses.COLOR_BLACK, curses.COLOR_RED)
    curses.init_pair(14, curses.COLOR_BLACK, curses.COLOR_MAGENTA)
    col_unknown = curses.color_pair(1)
    col_creature = curses.color_pair(13)
    col_player = curses.color_pair(8)
    col_altar = curses.color_pair(3)
    earth_colors = [
        curses.color_pair(4),
        curses.color_pair(5),
        curses.color_pair(5),
        curses.color_pair(6),
        curses.color_pair(6),
        curses.color_pair(7),
    ]
    water_colors = [
        curses.color_pair(9),
        curses.color_pair(10),
        curses.color_pair(11),
        curses.color_pair(12),
        curses.color_pair(14),
    ]
    for y in range(world_data["map_size"]):
        for x in range(world_data["map_size"]):
            pos = y * world_data["map_size"] + x
            char = world_data["fov_map"][pos]
            if world_data["look_mode"] and y == world_data["map_center"][0] \
                    and x == world_data["map_center"][1]:
                if char == " ":
                    bonus = "?"
                else:
                    bonus = world_data["wetmap"][pos]
                char = world_data["mem_map"][pos]
                winmap += [(char, curses.A_REVERSE), (bonus, curses.A_REVERSE)]
                continue
            bonus = " "
            attribute = col_unknown
            if char == " ":
                if world_data["soundmap"][pos] != "0":
                    bonus = "?"
                char = world_data["mem_map"][pos]
                if char in charmap:
                    char = charmap[char]
                winmap += [(char, attribute), bonus]
            else:
                wetval = ord(world_data["wetmap"][pos]) - ord("0")
                if char in "012345-+":
                    mapval = 0
                    if char not in "-+":
                        mapval = ord(char) - ord("0")
                    if 1 <= wetval <= 5:
                        attribute = water_colors[wetval + (mapval - 1)]
                    else:
                        attribute = earth_colors[mapval]
                if char == "&":
                    attribute = col_player
                elif char == "$":
                    attribute = col_altar
                if char in charmap:
                    char = charmap[char]
                elif char == "@":
                    attribute = col_creature
                    av_pos = world_data["avatar_position"]
                    if pos == av_pos[0] * world_data["map_size"] + av_pos[1]:
                        attribute = col_player
                winmap += [(char, attribute), bonus]
        if y % 2 == 0:
            winmap += "  "
    return offset, winmap_size, winmap


from client.config.world_data import world_data
world_data["grace"] = 0
world_data["kidney"] = 0
world_data["stomach"] = 0
world_data["bowel"] = 0
world_data["bladder"] = 0
world_data["wetmap"] = " " * (world_data["map_size"] ** 2)
world_data["soundmap"] = " " * (world_data["map_size"] ** 2)
world_data["log"] = [
"This is not the environment you are used to. This is not the body you are use"
"d to. Surely this is a dream. But you have a feeling you might not wake up. U"
"nless you find a way out of here. Until then, you must survive. Explore, keep"
" fed, and hydrated. And avoid things that move by themselves.",
"",
"[hit '0' for help]"
]
from client.config.io import io
io["worldstate_read_order"] += [["stomach", "int"]]
io["worldstate_read_order"] += [["kidney", "int"]]
io["worldstate_read_order"] += [["bowel", "int"]]
io["worldstate_read_order"] += [["bladder", "int"]]
io["worldstate_read_order"] += [["wetmap", "map"]]
io["worldstate_read_order"] += [["soundmap", "map"]]
io["worldstate_read_order"] += [["grace", "int"]]
from client.config.windows import windows_config
from client.windows import win_log
windows_config[:] = [
    {"config": [0, -33],
     "func": win_map,
     "scroll_hints": False,
      "title": "THE CRAWLING EATER"},
    {"config": [1, 32],
     "func": win_bar_maker(76, "+", "grace"),
     "scroll_hints": False,
      "title": "grace"},
    {"config": [1, 32],
     "func": win_bar_maker(77, "%", "stomach"),
     "scroll_hints": False,
     "title": "calories"},
    {"config": [1, 32],
     "func": win_bar_maker(79, "~", "kidney"),
     "scroll_hints": False,
      "title": "hydration"},
    {"config": [1, 32],
     "func": win_bar_maker(78, "%", "bowel"),
     "scroll_hints": False,
     "title": "bowel"},
    {"config": [1, 32],
     "func": win_bar_maker(80, "~", "bladder"),
     "scroll_hints": False,
      "title": "bladder"},
    {"config": [-10, 32],
     "func": win_log,
     "scroll_hints": False,
     "title": "log"}
]
from client.window_management import set_windows
set_windows()
from client.commands import command_sender
from client.config.commands import commands
commands["o"] = (command_sender("drop"),)
commands["i"] = (command_sender("drink"),)
commands["p"] = (command_sender("pee"),)
commands["0"] = (command_sender("HELP 0"),)
commands["1"] = (command_sender("HELP 1"),)
commands["2"] = (command_sender("HELP 2"),)
commands["3"] = (command_sender("HELP 3"),)
commands["4"] = (command_sender("HELP 4"),)
commands["5"] = (command_sender("HELP 5"),)
commands["6"] = (command_sender("HELP 6"),)
commands["7"] = (command_sender("HELP 7"),)
commands["8"] = (command_sender("HELP 8"),)
commands["9"] = (command_sender("HELP 9"),)
