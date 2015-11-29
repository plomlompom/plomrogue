import curses
import math
import os
import signal
import time


def set_window_geometries():

    def set_window_size():
        win["size"], win["start"] = [0, 0], [0, 0]
        win["size"][0] = win["config"][0]
        if (win["config"][0] == 0):
            win["size"][0] = screen_size[0] - sep_size
        elif (win["config"][0] < 0):
            win["size"][0] = screen_size[0] + win["config"][0] - sep_size
        win["size"][1] = win["config"][1]
        if (win["config"][1] == 0):
            win["size"][1] = screen_size[1]
        elif (win["config"][1] < 0):
            win["size"][1] = screen_size[1] + win["config"][1]

    def place_window():
        win_i = windows.index(win)

        # If win is first window, it goes into the top left corner.
        win["start"][0] = 0 + sep_size
        win["start"][1] = 0
        if (win_i > 0):

            # If not, get win's closest predecessor starting a new stack on the
            # screen top,fit win's top left to that win_top's top right corner.
            win_top = None
            for i in range(win_i - 1, -1, -1):
                win_top = windows[i]
                if (win_top["start"][0] == 0 + sep_size):
                    break
            win["start"][1] = win_top["start"][1] + win_top["size"][1] \
                + sep_size

            # If enough space is found below win's predecessor, fit win's top
            # left corner to that predecessor's bottom left corner.
            win_prev = windows[win_i - 1]
            next_free_y = win_prev["start"][0] + win_prev["size"][0] + sep_size
            if (win["size"][1] <= win_prev["size"][1] and
                    win["size"][0] <= screen_size[0] - next_free_y):
                win["start"][1] = win_prev["start"][1]
                win["start"][0] = next_free_y

            # If that fails, try to fit win's top left corner to the top right
            # corner of its closest predecessor win_test 1) below win_top (see
            # above) 2) and with enough space open to its right between its
            # right edge and the lower edge of a win_high located directly
            # above win_test to fit win there (without growing further to the
            # right than win_high does or surpassing the screen's lower edge).
            else:
                win_test = win_prev
                win_high = None
                while (win_test != win_top):
                    for i in range(win_i - 2, -1, -1):
                        win_high = windows[i]
                        if win_test["start"][0] > win_high["start"][0]:
                            break
                    next_free_y = win_high["start"][0] + win_high["size"][0] \
                        + sep_size
                    first_free_x = win_test["start"][1] + win_test["size"][1] \
                        + sep_size
                    last_free_x = win_high["start"][1] + win_high["size"][1]
                    if (win["size"][0] <= screen_size[0] - next_free_y and
                            win["size"][1] <= last_free_x - first_free_x):
                        win["start"][1] = first_free_x
                        win["start"][0] = next_free_y
                        break
                    win_test = win_high

    global screen_size, stdscr
    curses.endwin()
    stdscr = curses.initscr()
    screen_size = stdscr.getmaxyx()
    for win in windows:
        set_window_size()
        place_window()
    cursed_main.redraw = True


def draw_screen():

    def healthy_addch(y, x, char, attr=0):
        """Workaround for <http://stackoverflow.com/questions/7063128/>."""
        if y == screen_size[0] - 1 and x == screen_size[1] - 1:
            char_before = stdscr.inch(y, x - 1)
            stdscr.addch(y, x - 1, char, attr)
            stdscr.insstr(y, x - 1, " ")
            stdscr.addch(y, x - 1, char_before)
        else:
            stdscr.addch(y, x, char, attr)

    def draw_window_border_lines():
        for win in windows:
            for k in range(2):
                j = win["start"][int(k == 0)] - sep_size
                if (j >= 0 and j < screen_size[int(k == 0)]):
                    start = win["start"][k]
                    # start = start if start >= 0 else 0
                    end = win["start"][k] + win["size"][k]
                    end = end if end < screen_size[k] else screen_size[k]
                    if k:
                        [healthy_addch(j, i, '-') for i in range(start, end)]
                    else:
                        [healthy_addch(i, j, '|') for i in range(start, end)]

    def draw_window_border_corners():
        for win in windows:
            up = win["start"][0] - sep_size
            down = win["start"][0] + win["size"][0]
            left = win["start"][1] - sep_size
            right = win["start"][1] + win["size"][1]
            if (up >= 0 and up < screen_size[0]):
                if (left >= 0 and left < screen_size[1]):
                    healthy_addch(up, left, '+')
                if (right >= 0 and right < screen_size[1]):
                    healthy_addch(up, right, '+')
            if (down >= 0 and down < screen_size[0]):
                if (left >= 0 and left < screen_size[1]):
                    healthy_addch(down, left, '+')
                if (right >= 0 and right < screen_size[1]):
                    healthy_addch(down, right, '+')

    def draw_window_contents():
        def draw_winmap():
            stop = [0, 0]
            for i in range(2):
                stop[i] = win["size"][i] + offset[i]
                if stop[i] >= winmap_size[i]:
                    stop[i] = winmap_size[i]
            for y in range(offset[0], stop[0]):
                for x in range(offset[1], stop[1]):
                    cell = winmap[y * winmap_size[1] + x]
                    attr = 0
                    if len(cell) > 1:
                        attr = cell[1]
                        cell = cell[0]
                    y_in_screen = win["start"][0] + (y - offset[0])
                    x_in_screen = win["start"][1] + (x - offset[1])
                    if (y_in_screen < screen_size[0]
                            and x_in_screen < screen_size[1]):
                        healthy_addch(y_in_screen, x_in_screen, cell, attr)
        def draw_scroll_hints():
            def draw_scroll_string(n_lines_outside):
                hint = ' ' + str(n_lines_outside + 1) + ' more ' + unit + ' '
                if len(hint) <= win["size"][ni]:
                    non_hint_space = win["size"][ni] - len(hint)
                    hint_offset = int(non_hint_space / 2)
                    for j in range(win["size"][ni] - non_hint_space):
                        pos_2 = win["start"][ni] + hint_offset + j
                        x, y = (pos_2, pos_1) if ni else (pos_1, pos_2)
                        healthy_addch(y, x, hint[j], curses.A_REVERSE)
            def draw_scroll_arrows(ar1, ar2):
                for j in range(win["size"][ni]):
                    pos_2 = win["start"][ni] + j
                    x, y = (pos_2, pos_1) if ni else (pos_1, pos_2)
                    healthy_addch(y, x, ar1 if ni else ar2, curses.A_REVERSE)
            for i in range(2):
                ni = int(i == 0)
                unit = 'rows' if ni else 'columns'
                if (offset[i] > 0):
                    pos_1 = win["start"][i]
                    draw_scroll_arrows('^', '<')
                    draw_scroll_string(offset[i])
                if (winmap_size[i] > offset[i] + win["size"][i]):
                    pos_1 = win["start"][i] + win["size"][i] - 1
                    draw_scroll_arrows('v', '>')
                    draw_scroll_string(winmap_size[i] - offset[i]
                        - win["size"][i])
        for win in windows:
            offset, winmap_size, winmap = win["func"]()
            draw_winmap()
            draw_scroll_hints()

    stdscr.clear()
    draw_window_border_lines()
    draw_window_border_corners()
    draw_window_contents()
    stdscr.refresh()


def read_worldstate():
    if not os.access(io["path_worldstate"], os.F_OK):
        msg = "No world state file found at " + io["path_worldstate"] + "."
        raise SystemExit(msg)
    read_anew = False
    worldstate_file = open(io["path_worldstate"], "r")
    turn_string = worldstate_file.readline()
    if int(turn_string) != world_data["turn"]:
        read_anew = True
    if not read_anew: # In rare cases, world may change, but not turn number.
        mtime = os.stat(io["path_worldstate"])
        if mtime != read_worldstate.last_checked_mtime:
            read_worldstate.last_checked_mtime = mtime
            read_anew = True
    if read_anew:
        cursed_main.redraw = True
        world_data["turn"] = int(turn_string)
        world_data["lifepoints"] = int(worldstate_file.readline())
        world_data["satiation"] = int(worldstate_file.readline())
        world_data["inventory"] = []
        while True:
            line = worldstate_file.readline().replace("\n", "")
            if line == '%':
                break
            world_data["inventory"] += [line]
        world_data["avatar_position"][0] = int(worldstate_file.readline())
        world_data["avatar_position"][1] = int(worldstate_file.readline())
        if not world_data["look_mode"]:
            world_data["map_center"][0] = world_data["avatar_position"][0]
            world_data["map_center"][1] = world_data["avatar_position"][1]
        world_data["map_size"] = int(worldstate_file.readline())
        world_data["fov_map"] = ""
        for i in range(world_data["map_size"]):
            line = worldstate_file.readline().replace("\n", "")
            world_data["fov_map"] += line
        world_data["mem_map"] = ""
        for i in range(world_data["map_size"]):
            line = worldstate_file.readline().replace("\n", "")
            world_data["mem_map"] += line
    worldstate_file.close()
read_worldstate.last_checked_mtime = -1


def read_message_queue():
    while (len(message_queue["messages"]) > 1
        or (len(message_queue["messages"]) == 0
            and not message_queue["open_end"])):
        message = message_queue["messages"].pop(0)
        if message[0:4] == "LOG ":
            world_data["log"] += [message[4:]]
            cursed_main.redraw = True


def cursed_main(stdscr):

    def ping_test():
        half_wait_time = 5
        if len(new_data_from_server) > 0:
            ping_test.sent = False
        elif ping_test.wait_start + half_wait_time < time.time():
            if not ping_test.sent:
                io["file_out"].write("PING\n")
                io["file_out"].flush()
                ping_test.sent = True
                ping_test.wait_start = time.time()
            elif ping_test.sent:
                raise SystemExit("Server not answering anymore.")
    ping_test.wait_start = 0

    def read_into_message_queue():
        if new_data_from_server == "":
            return
        new_open_end = False
        if new_data_from_server[-1] is not "\n":
            new_open_end = True
        new_messages = new_data_from_server.splitlines()
        if message_queue["open_end"]:
            message_queue["messages"][-1] += new_messages[0]
            del new_messages[0]
        message_queue["messages"] += new_messages
        if new_open_end:
            message_queue["open_end"] = True

    curses.noecho()
    curses.curs_set(False)
    # stdscr.keypad(True)
    signal.signal(signal.SIGWINCH,
        lambda ignore_1, ignore_2: set_window_geometries())
    set_window_geometries()
    delay = 1
    while True:
        stdscr.timeout(delay)
        delay = delay * 2 if delay < 1000 else delay
        if cursed_main.redraw:
            delay = 1
            draw_screen()
            cursed_main.redraw = False
        char = stdscr.getch()
        if char >= 0 and chr(char) in commands:
            commands[chr(char)]()
            cursed_main.redraw = True
        new_data_from_server = io["file_in"].read()
        ping_test()
        read_into_message_queue()
        read_worldstate()
        read_message_queue()


def win_foo():
    winmap = [('.', 0), ('o', 0), ('.', 0), ('o', 0), ('O', 0), ('o', 0),
        ('.', 0), ('o', 0), ('.', 0), ('x', 0), ('y', 0), ('x', 0)]
    winmap_size = [4, 3]
    offset = [0, 0]
    return offset, winmap_size, winmap


def win_map():
    win_size = next(win["size"] for win in windows if win["func"] == win_map)
    offset = [0, 0]
    for i in range(2):
        if world_data["map_center"][i] * (i + 1) > win_size[i] / 2:
            if world_data["map_center"][i] * (i + 1) \
                < world_data["map_size"] * (i + 1) - win_size[i] / 2:
                offset[i] = world_data["map_center"][i] * (i + 1) \
                    - int(win_size[i] / 2)
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


def win_inventory():
    winmap = ""
    winmap_size = [0, 0]
    for line in world_data["inventory"]:
        winmap_size[1] = winmap_size[1] if len(line) <= winmap_size[1] \
            else len(line)
    for line in world_data["inventory"]:
        padding_size = winmap_size[1] - len(line)
        winmap += line + (" " * padding_size)
        winmap_size[0] = winmap_size[0] + 1
    offset = [0, 0]
    return offset, winmap_size, winmap


def win_info():
    winmap = "T: " + str(world_data["turn"]) \
        + " H: " + str(world_data["lifepoints"]) \
        + " S: " + str(world_data["satiation"])
    winmap_size = [1, len(winmap)]
    offset = [0, 0]
    return offset, winmap_size, winmap


def win_log():
    win_size = next(win["size"] for win in windows if win["func"] == win_log)
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


def command_quit():
    io["file_out"].write("QUIT\n")
    io["file_out"].flush()
    raise SystemExit("Received QUIT command, forwarded to server, leaving.")


def command_toggle_look_mode():
    world_data["look_mode"] = False if world_data["look_mode"] else True


windows = [
    {"config": [1, 33], "func": win_info},
    {"config": [-7, 33], "func": win_log},
    {"config": [4, 16], "func": win_inventory},
    {"config": [4, 16], "func": win_foo},
    {"config": [0, -34], "func": win_map}
]
io = {
    "path_out": "server/in",
    "path_in": "server/out",
    "path_worldstate": "server/worldstate"
}
commands = {
    "l": command_toggle_look_mode,
    "Q": command_quit
}
message_queue = {
    "open_end": False,
    "messages": []
}
world_data = {
    "avatar_position": [-1, -1],
    "fov_map": "",
    "inventory": [],
    "lifepoints": -1,
    "look_mode": False,
    "log": [],
    "map_center": [-1, -1],
    "map_size": 0,
    "mem_map": "",
    "satiation": -1,
    "turn": -1
}
sep_size = 1  # Width of inter-window borders and title bars.
stdscr = None
screen_size = [0,0]


try:
    if (not os.access(io["path_out"], os.F_OK)):
        msg = "No server input file found at " + io["path_out"] + "."
        raise SystemExit(msg)
    io["file_out"] = open(io["path_out"], "a")
    io["file_in"] = open(io["path_in"], "r")
    curses.wrapper(cursed_main)
except SystemExit as exit:
    print("ABORTING: " + exit.args[0])
except:
    print("SOMETHING WENT WRONG IN UNEXPECTED WAYS")
    raise
finally:
    if "file_out" in io:
        io["file_out"].close()
    if "file_in" in io:
        io["file_in"].close()
