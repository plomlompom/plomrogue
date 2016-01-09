import curses
import types


from client.config.windows import windows_config


redraw_windows = False
sep_size = 1  # Width of inter-window borders and title bars.
windows = []
stdscr = None
screen_size = [0,0]


class Window:
    def __init__(self, title, draw_function, size):
        self.title = title
        self.draw = types.MethodType(draw_function, self)
        self.size = size


def set_window_geometries():

    def size_window(config):
        size = [0, 0]
        size[0] = config[0]
        if (config[0] == 0):
            size[0] = screen_size[0] - sep_size
        elif (config[0] < 0):
            size[0] = screen_size[0] + config[0] - sep_size
        size[1] = config[1]
        if (config[1] == 0):
            size[1] = screen_size[1]
        elif (config[1] < 0):
            size[1] = screen_size[1] + config[1]
        return size

    def place_window(win):
        win_i = windows.index(win)

        # If win is first window, it goes into the top left corner.
        win.start = [0 + sep_size, 0]
        if (win_i > 0):

            # If not, get win's closest predecessor starting a new stack on the
            # screen top,fit win's top left to that win_top's top right corner.
            win_top = None
            for i in range(win_i - 1, -1, -1):
                win_top = windows[i]
                if (win_top.start[0] == 0 + sep_size):
                    break
            win.start[1] = win_top.start[1] + win_top.size[1] + sep_size

            # If enough space is found below win's predecessor, fit win's top
            # left corner to that predecessor's bottom left corner.
            win_prev = windows[win_i - 1]
            next_free_y = win_prev.start[0] + win_prev.size[0] + sep_size
            if (win.size[1] <= win_prev.size[1] and
                    win.size[0] <= screen_size[0] - next_free_y):
                win.start[1] = win_prev.start[1]
                win.start[0] = next_free_y

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
                        if win_test.start[0] > win_high.start[0]:
                            break
                    next_free_y = win_high.start[0] + win_high.size[0] \
                        + sep_size
                    first_free_x = win_test.start[1] + win_test.size[1] \
                        + sep_size
                    last_free_x = win_high.start[1] + win_high.size[1]
                    if (win.size[0] <= screen_size[0] - next_free_y and
                            win.size[1] <= last_free_x - first_free_x):
                        win.start[1] = first_free_x
                        win.start[0] = next_free_y
                        break
                    win_test = win_high

    global screen_size, stdscr
    curses.endwin()
    stdscr = curses.initscr()
    screen_size = stdscr.getmaxyx()
    for config in windows_config:
        size = size_window(config["config"])
        window = Window(config["title"], config["func"], size)
        windows.append(window)
        place_window(window)
    redraw_windows = True


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
                j = win.start[int(k == 0)] - sep_size
                if (j >= 0 and j < screen_size[int(k == 0)]):
                    start = win.start[k]
                    end = win.start[k] + win.size[k]
                    end = end if end < screen_size[k] else screen_size[k]
                    if k:
                        [healthy_addch(j, i, '-') for i in range(start, end)]
                    else:
                        [healthy_addch(i, j, '|') for i in range(start, end)]

    def draw_window_border_corners():
        for win in windows:
            up = win.start[0] - sep_size
            down = win.start[0] + win.size[0]
            left = win.start[1] - sep_size
            right = win.start[1] + win.size[1]
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

    def draw_window_titles():
        for win in windows:
            title = " " + win.title + " "
            if len(title) <= win.size[1]:
                y = win.start[0] - 1
                start_x = win.start[1] + int((win.size[1] - len(title))/2)
                for x in range(len(title)):
                    healthy_addch(y, start_x + x, title[x])

    def draw_window_contents():
        def draw_winmap():
            """Draw winmap in area delimited by offset, winmap_size.

            The individuall cell of a winmap is interpreted as either a single
            character element, or as a tuple of character and attribute,
            depending on the size len(cell) spits out.
            """
            stop = [0, 0]
            for i in range(2):
                stop[i] = win.size[i] + offset[i]
                if stop[i] >= winmap_size[i]:
                    stop[i] = winmap_size[i]
            for y in range(offset[0], stop[0]):
                for x in range(offset[1], stop[1]):
                    cell = winmap[y * winmap_size[1] + x]
                    attr = 0
                    if len(cell) > 1:
                        attr = cell[1]
                        cell = cell[0]
                    y_in_screen = win.start[0] + (y - offset[0])
                    x_in_screen = win.start[1] + (x - offset[1])
                    if (y_in_screen < screen_size[0]
                            and x_in_screen < screen_size[1]):
                        healthy_addch(y_in_screen, x_in_screen, cell, attr)
        def draw_scroll_hints():
            def draw_scroll_string(n_lines_outside):
                hint = ' ' + str(n_lines_outside + 1) + ' more ' + unit + ' '
                if len(hint) <= win.size[ni]:
                    non_hint_space = win.size[ni] - len(hint)
                    hint_offset = int(non_hint_space / 2)
                    for j in range(win.size[ni] - non_hint_space):
                        pos_2 = win.start[ni] + hint_offset + j
                        x, y = (pos_2, pos_1) if ni else (pos_1, pos_2)
                        healthy_addch(y, x, hint[j], curses.A_REVERSE)
            def draw_scroll_arrows(ar1, ar2):
                for j in range(win.size[ni]):
                    pos_2 = win.start[ni] + j
                    x, y = (pos_2, pos_1) if ni else (pos_1, pos_2)
                    healthy_addch(y, x, ar1 if ni else ar2, curses.A_REVERSE)
            for i in range(2):
                ni = int(i == 0)
                unit = 'rows' if ni else 'columns'
                if (offset[i] > 0):
                    pos_1 = win.start[i]
                    draw_scroll_arrows('^', '<')
                    draw_scroll_string(offset[i])
                if (winmap_size[i] > offset[i] + win.size[i]):
                    pos_1 = win.start[i] + win.size[i] - 1
                    draw_scroll_arrows('v', '>')
                    draw_scroll_string(winmap_size[i] - offset[i]
                        - win.size[i])
        for win in windows:
            offset, winmap_size, winmap = win.draw()
            draw_winmap()
            draw_scroll_hints()

    stdscr.erase()
    draw_window_border_lines()
    draw_window_border_corners()
    draw_window_titles()
    draw_window_contents()
    stdscr.refresh()
