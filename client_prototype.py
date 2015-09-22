import curses


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

    for win in windows:
        set_window_size()
        place_window()


def draw_screen():

    def draw_window_border_lines():
        for win in windows:
            for k in range(2):
                j = win["start"][int(k == 0)] - sep_size
                if (j >= 0 and j < screen_size[int(k == 0)]):
                    start = win["start"][k]
                    start = start if start >= 0 else 0
                    end = win["start"][k] + win["size"][k]
                    end = end if end < screen_size[k] else screen_size[k]
                    if k:
                        [stdscr.addch(j, i, '-') for i in range(start, end)]
                    else:
                        [stdscr.addch(i, j, '|') for i in range(start, end)]

    def draw_window_border_corners():
        for win in windows:
            up = win["start"][0] - sep_size
            down = win["start"][0] + win["size"][0]
            left = win["start"][1] - sep_size
            right = win["start"][1] + win["size"][1]
            if (up >= 0 and up < screen_size[0]):
                if (left >= 0 and left < screen_size[1]):
                    stdscr.addch(up, left, '+')
                if (right >= 0 and right < screen_size[1]):
                    stdscr.addch(up, right, '+')
            if (down >= 0 and down < screen_size[0]):
                if (left >= 0 and left < screen_size[1]):
                    stdscr.addch(down, left, '+')
                if (right >= 0 and right < screen_size[1]):
                    stdscr.addch(down, right, '+')

    def draw_window_contents():
        for win in windows:
            offset, size, winmap = win["func"]()
            stop = [0, 0]
            for i in range(2):
                stop[i] = win["size"][i] + offset[i]
                stop[i] = stop[i] if stop[i] < size[i] else size[i]
            for y in range(offset[0], stop[0]):
                for x in range(offset[1], stop[1]):
                    cell = winmap[y * size[1] + x]
                    y_in_screen = win["start"][0] + (y - offset[0])
                    x_in_screen = win["start"][1] + (x - offset[1])
                    if (y_in_screen < screen_size[0]
                            and x_in_screen < screen_size[1]):
                        stdscr.addch(y_in_screen, x_in_screen, cell)

    draw_window_border_lines()
    draw_window_border_corners()
    draw_window_contents()
    stdscr.refresh()


def main(stdscr):
    curses.noecho()
    curses.curs_set(False)
    # stdscr.keypad(True)
    set_window_geometries()
    while True:
        draw_screen()
        stdscr.getch()


def foo():
    winmap = ['.', 'o', '.', 'o', 'O', 'o', '.', 'o', '.', 'x', 'y', 'x']
    size = [4, 3]
    offset = [0, 0]
    return offset, size, winmap


windows = [
    {"config": [1, 33], "func": foo},
    {"config": [-7, 33], "func": foo},
    {"config": [4, 16], "func": foo},
    {"config": [4, 16], "func": foo},
    {"config": [0, -34], "func": foo}
]

sep_size = 1  # Width of inter-window borders and title bars.
stdscr = curses.initscr()
screen_size = stdscr.getmaxyx()

curses.wrapper(main)
