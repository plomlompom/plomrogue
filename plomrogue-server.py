import argparse
import errno
import os
import shlex
import shutil
import time
import ctypes


class RandomnessIO:
    """"Interface to libplomrogue's pseudo-randomness generator."""

    def set_seed(self, seed):
        libpr.seed_rrand(1, seed)

    def get_seed(self):
        return libpr.seed_rrand(0, 0)

    def next(self):
        return libpr.rrand()

    seed = property(get_seed, set_seed)


def prep_library():
    """Prepare ctypes library at ./libplomrogue.so"""
    libpath = ("./libplomrogue.so")
    if not os.access(libpath, os.F_OK):
        raise SystemExit("No library " + libpath + ", run ./compile.sh first?")
    libpr = ctypes.cdll.LoadLibrary(libpath)
    libpr.seed_rrand.argtypes = [ctypes.c_uint8, ctypes.c_uint32]
    libpr.seed_rrand.restype = ctypes.c_uint32
    libpr.rrand.argtypes = []
    libpr.rrand.restype = ctypes.c_uint16
    libpr.set_maplength.argtypes = [ctypes.c_uint16]
    libpr.mv_yx_in_dir_legal_wrap.argtypes = [ctypes.c_char, ctypes.c_uint8,
                                              ctypes.c_uint8]
    libpr.mv_yx_in_dir_legal_wrap.restype = ctypes.c_uint8
    libpr.result_y.restype = ctypes.c_uint8
    libpr.result_x.restype = ctypes.c_uint8
    libpr.set_maplength(world_db["MAP_LENGTH"])
    libpr.build_fov_map.argtypes = [ctypes.c_uint8, ctypes.c_uint8,
                                    ctypes.c_char_p, ctypes.c_char_p]
    libpr.build_fov_map.restype = ctypes.c_uint8
    return libpr


def strong_write(file, string):
    """Apply write(string), flush(), and os.fsync() to file."""
    file.write(string)
    file.flush()
    os.fsync(file)


def setup_server_io():
    """Fill IO files DB with proper file( path)s. Write process IO test string.

    Ensure IO files directory at server/. Remove any old input file if found.
    Set up new input file for reading, and new output file for writing. Start
    output file with process hash line of format PID + " " + floated UNIX time
    (io_db["teststring"]). Raise SystemExit if file is found at path of either
    record or save file plus io_db["tmp_suffix"].
    """
    def detect_atomic_leftover(path, tmp_suffix):
        path_tmp = path + tmp_suffix
        msg = "Found file '" + path_tmp + "' that may be a leftover from an " \
              "aborted previous attempt to write '" + path + "'. Aborting " \
             "until matter is resolved by removing it from its current path."
        if os.access(path_tmp, os.F_OK):
            raise SystemExit(msg)
    io_db["teststring"] = str(os.getpid()) + " " + str(time.time())
    os.makedirs(io_db["path_server"], exist_ok=True)
    io_db["file_out"] = open(io_db["path_out"], "w")
    strong_write(io_db["file_out"], io_db["teststring"] + "\n")
    if os.access(io_db["path_in"], os.F_OK):
        os.remove(io_db["path_in"])
    io_db["file_in"] = open(io_db["path_in"], "w")
    io_db["file_in"].close()
    io_db["file_in"] = open(io_db["path_in"], "r")
    detect_atomic_leftover(io_db["path_save"], io_db["tmp_suffix"])
    detect_atomic_leftover(io_db["path_record"], io_db["tmp_suffix"])


def cleanup_server_io():
    """Close and (if io_db["kicked_by_rival"] false) remove files in io_db."""
    def helper(file_key, path_key):
        if file_key in io_db:
            io_db[file_key].close()
            if not io_db["kicked_by_rival"] \
               and os.access(io_db[path_key], os.F_OK):
                os.remove(io_db[path_key])
    helper("file_out", "path_out")
    helper("file_in", "path_in")
    helper("file_worldstate", "path_worldstate")
    if "file_record" in io_db:
        io_db["file_record"].close()


def obey(command, prefix, replay=False, do_record=False):
    """Call function from commands_db mapped to command's first token.

    Tokenize command string with shlex.split(comments=True). If replay is set,
    a non-meta command from the commands_db merely triggers obey() on the next
    command from the records file. If not, non-meta commands set
    io_db["worldstate_updateable"] to world_db["WORLD_EXISTS"], and, if
    do_record is set, are recorded via record(), and save_world() is called.
    The prefix string is inserted into the server's input message between its
    beginning 'input ' & ':'. All activity is preceded by a server_test() call.
    """
    server_test()
    print("input " + prefix + ": " + command)
    try:
        tokens = shlex.split(command, comments=True)
    except ValueError as err:
        print("Can't tokenize command string: " + str(err) + ".")
        return
    if len(tokens) > 0 and tokens[0] in commands_db \
       and len(tokens) == commands_db[tokens[0]][0] + 1:
        if commands_db[tokens[0]][1]:
            commands_db[tokens[0]][2](*tokens[1:])
        elif replay:
            print("Due to replay mode, reading command as 'go on in record'.")
            line = io_db["file_record"].readline()
            if len(line) > 0:
                obey(line.rstrip(), io_db["file_record"].prefix
                     + str(io_db["file_record"].line_n))
                io_db["file_record"].line_n = io_db["file_record"].line_n + 1
            else:
                print("Reached end of record file.")
        else:
            commands_db[tokens[0]][2](*tokens[1:])
            if do_record:
                record(command)
                save_world()
            io_db["worldstate_updateable"] = world_db["WORLD_ACTIVE"]
    elif 0 != len(tokens):
        print("Invalid command/argument, or bad number of tokens.")


def atomic_write(path, text, do_append=False):
    """Atomic write of text to file at path, appended if do_append is set."""
    path_tmp = path + io_db["tmp_suffix"]
    mode = "w"
    if do_append:
        mode = "a"
        if os.access(path, os.F_OK):
            shutil.copyfile(path, path_tmp)
    file = open(path_tmp, mode)
    strong_write(file, text)
    file.close()
    if os.access(path, os.F_OK):
        os.remove(path)
    os.rename(path_tmp, path)


def record(command):
    """Append command string plus newline to record file. (Atomic.)"""
    # This misses some optimizations from the original record(), namely only
    # finishing the atomic write with expensive flush() and fsync() every 15
    # seconds unless explicitely forced. Implement as needed.
    atomic_write(io_db["path_record"], command + "\n", do_append=True)


def save_world():
    """Save all commands needed to reconstruct current world state."""
    # TODO: Misses same optimizations as record() from the original record().

    def quote(string):
        string = string.replace("\u005C", '\u005C\u005C')
        return '"' + string.replace('"', '\u005C"') + '"'

    def mapsetter(key):
        def helper(id):
            string = ""
            if world_db["Things"][id][key]:
                map = world_db["Things"][id][key]
                length = world_db["MAP_LENGTH"]
                for i in range(length):
                    line = map[i * length:(i * length) + length].decode()
                    string = string + key + " " + str(i) + " " + quote(line) \
                             + "\n"
            return string
        return helper

    def memthing(id):
        string = ""
        for memthing in world_db["Things"][id]["T_MEMTHING"]:
            string = string + "T_MEMTHING " + str(memthing[0]) + " " + \
                     str(memthing[1]) + " " + str(memthing[2]) + "\n"
        return string

    def helper(category, id_string, special_keys={}):
        string = ""
        for id in world_db[category]:
            string = string + id_string + " " + str(id) + "\n"
            for key in world_db[category][id]:
                if not key in special_keys:
                    x = world_db[category][id][key]
                    argument = quote(x) if str == type(x) else str(x)
                    string = string + key + " " + argument + "\n"
                elif special_keys[key]:
                    string = string + special_keys[key](id)
        return string

    string = ""
    for key in world_db:
        if dict != type(world_db[key]) and key != "MAP":
            string = string + key + " " + str(world_db[key]) + "\n"
    string = string + helper("ThingActions", "TA_ID")
    string = string + helper("ThingTypes", "TT_ID", {"TT_CORPSE_ID": False})
    for id in world_db["ThingTypes"]:
        string = string + "TT_ID " + str(id) + "\n" + "TT_CORPSE_ID " + \
                 str(world_db["ThingTypes"][id]["TT_CORPSE_ID"]) + "\n"
    string = string + helper("Things", "T_ID",
                             {"T_CARRIES": False, "carried": False,
                              "T_MEMMAP": mapsetter("T_MEMMAP"),
                              "T_MEMTHING": memthing, "fovmap": False,
                              "T_MEMDEPTHMAP": mapsetter("T_MEMDEPTHMAP")})
    for id in world_db["Things"]:
        if [] != world_db["Things"][id]["T_CARRIES"]:
            string = string + "T_ID " + str(id) + "\n"
            for carried_id in world_db["Things"][id]["T_CARRIES"]:
                string = string + "T_CARRIES " + str(carried_id) + "\n"
    string = string + "SEED_RANDOMNESS " + str(rand.seed) + "\n" + \
             "WORLD_ACTIVE " + str(world_db["WORLD_ACTIVE"])
    atomic_write(io_db["path_save"], string)


def obey_lines_in_file(path, name, do_record=False):
    """Call obey() on each line of path's file, use name in input prefix."""
    file = open(path, "r")
    line_n = 1
    for line in file.readlines():
        obey(line.rstrip(), name + "file line " + str(line_n),
             do_record=do_record)
        line_n = line_n + 1
    file.close()


def parse_command_line_arguments():
    """Return settings values read from command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', nargs='?', type=int, dest='replay', const=1,
                        action='store')
    opts, unknown = parser.parse_known_args()
    return opts


def server_test():
    """Ensure valid server out file belonging to current process.

    This is done by comparing io_db["teststring"] to what's found at the start
    of the current file at io_db["path_out"]. On failure, set
    io_db["kicked_by_rival"] and raise SystemExit.
    """
    if not os.access(io_db["path_out"], os.F_OK):
        raise SystemExit("Server output file has disappeared.")
    file = open(io_db["path_out"], "r")
    test = file.readline().rstrip("\n")
    file.close()
    if test != io_db["teststring"]:
        io_db["kicked_by_rival"] = True
        msg = "Server test string in server output file does not match. This" \
              " indicates that the current server process has been " \
              "superseded by another one."
        raise SystemExit(msg)


def read_command():
    """Return next newline-delimited command from server in file.

    Keep building return string until a newline is encountered. Pause between
    unsuccessful reads, and after too much waiting, run server_test().
    """
    wait_on_fail = 0.03333
    max_wait = 5
    now = time.time()
    command = ""
    while True:
        add = io_db["file_in"].readline()
        if len(add) > 0:
            command = command + add
            if len(command) > 0 and "\n" == command[-1]:
                command = command[:-1]
                break
        else:
            time.sleep(wait_on_fail)
            if now + max_wait < time.time():
                server_test()
                now = time.time()
    return command


def try_worldstate_update():
    """Write worldstate file if io_db["worldstate_updateable"] is set."""
    if io_db["worldstate_updateable"]:

        def draw_visible_Things(map, run):
            for id in world_db["Things"]:
                type = world_db["Things"][id]["T_TYPE"]
                consumable = world_db["ThingTypes"][type]["TT_CONSUMABLE"]
                alive = world_db["ThingTypes"][type]["TT_LIFEPOINTS"]
                if (0 == run and not consumable and not alive) \
                   or (1 == run and consumable and not alive) \
                   or (2 == run and alive):
                    y = world_db["Things"][id]["T_POSY"]
                    x = world_db["Things"][id]["T_POSX"]
                    fovflag = world_db["Things"][0]["fovmap"][(y * length) + x]
                    if 'v' == chr(fovflag):
                        c = world_db["ThingTypes"][type]["TT_SYMBOL"]
                        map[(y * length) + x] = ord(c)

        def write_map(string, map):
            for i in range(length):
                line = map[i * length:(i * length) + length].decode()
                string = string + line + "\n"
            return string

        inventory = ""
        if [] == world_db["Things"][0]["T_CARRIES"]:
            inventory = "(none)\n"
        else:
            for id in world_db["Things"][0]["T_CARRIES"]:
                type_id = world_db["Things"][id]["T_TYPE"]
                name = world_db["ThingTypes"][type_id]["TT_NAME"]
                inventory = inventory + name + "\n"
        string = str(world_db["TURN"]) + "\n" + \
                 str(world_db["Things"][0]["T_LIFEPOINTS"]) + "\n" + \
                 str(world_db["Things"][0]["T_SATIATION"]) + "\n" + \
                 inventory + "%\n" + \
                 str(world_db["Things"][0]["T_POSY"]) + "\n" + \
                 str(world_db["Things"][0]["T_POSX"]) + "\n" + \
                 str(world_db["MAP_LENGTH"]) + "\n"
        length = world_db["MAP_LENGTH"]
        fov = bytearray(b' ' * (length ** 2))
        for pos in range(length ** 2):
            if 'v' == chr(world_db["Things"][0]["fovmap"][pos]):
                fov[pos] = world_db["MAP"][pos]
        for i in range(3):
            draw_visible_Things(fov, i)
        string = write_map(string, fov)
        mem = world_db["Things"][0]["T_MEMMAP"][:]
        for i in range(2):
            for mt in world_db["Things"][0]["T_MEMTHING"]:
                consumable = world_db["ThingTypes"][mt[0]]["TT_CONSUMABLE"]
                if (i == 0 and not consumable) or (i == 1 and consumable):
                    c = world_db["ThingTypes"][mt[0]]["TT_SYMBOL"]
                    mem[(mt[1] * length) + mt[2]] = ord(c)
        string = write_map(string, mem)
        atomic_write(io_db["path_worldstate"], string)
        strong_write(io_db["file_out"], "WORLD_UPDATED\n")
        io_db["worldstate_updateable"] = False


def replay_game():
    """Replay game from record file.

    Use opts.replay as breakpoint turn to which to replay automatically before
    switching to manual input by non-meta commands in server input file
    triggering further reads of record file. Ensure opts.replay is at least 1.
    Run try_worldstate_update() before each interactive obey()/read_command().
    """
    if opts.replay < 1:
        opts.replay = 1
    print("Replay mode. Auto-replaying up to turn " + str(opts.replay) +
          " (if so late a turn is to be found).")
    if not os.access(io_db["path_record"], os.F_OK):
        raise SystemExit("No record file found to replay.")
    io_db["file_record"] = open(io_db["path_record"], "r")
    io_db["file_record"].prefix = "record file line "
    io_db["file_record"].line_n = 1
    while world_db["TURN"] < opts.replay:
        line = io_db["file_record"].readline()
        if "" == line:
            break
        obey(line.rstrip(), io_db["file_record"].prefix
             + str(io_db["file_record"].line_n))
        io_db["file_record"].line_n = io_db["file_record"].line_n + 1
    while True:
        try_worldstate_update()
        obey(read_command(), "in file", replay=True)


def play_game():
    """Play game by server input file commands. Before, load save file found.

    If no save file is found, a new world is generated from the commands in the
    world config plus a 'MAKE WORLD [current Unix timestamp]'. Record this
    command and all that follow via the server input file. Run
    try_worldstate_update() before each interactive obey()/read_command().
    """
    if os.access(io_db["path_save"], os.F_OK):
        obey_lines_in_file(io_db["path_save"], "save")
    else:
        if not os.access(io_db["path_worldconf"], os.F_OK):
            msg = "No world config file from which to start a new world."
            raise SystemExit(msg)
        obey_lines_in_file(io_db["path_worldconf"], "world config ",
                           do_record=True)
        obey("MAKE_WORLD " + str(int(time.time())), "in file", do_record=True)
    while True:
        try_worldstate_update()
        obey(read_command(), "in file", do_record=True)


def remake_map():
    """(Re-)make island map.

    Let "~" represent water, "." land, "X" trees: Build island shape randomly,
    start with one land cell in the middle, then go into cycle of repeatedly
    selecting a random sea cell and transforming it into land if it is neighbor
    to land. The cycle ends when a land cell is due to be created at the map's
    border. Then put some trees on the map (TODO: more precise algorithm desc).
    """
    def is_neighbor(coordinates, type):
        y = coordinates[0]
        x = coordinates[1]
        length = world_db["MAP_LENGTH"]
        ind = y % 2
        diag_west = x + (ind > 0)
        diag_east = x + (ind < (length - 1))
        pos = (y * length) + x
        if (y > 0 and diag_east
            and type == chr(world_db["MAP"][pos - length + ind])) \
           or (x < (length - 1)
               and type == chr(world_db["MAP"][pos + 1])) \
           or (y < (length - 1) and diag_east
               and type == chr(world_db["MAP"][pos + length + ind])) \
           or (y > 0 and diag_west
               and type == chr(world_db["MAP"][pos - length - (not ind)])) \
           or (x > 0
               and type == chr(world_db["MAP"][pos - 1])) \
           or (y < (length - 1) and diag_west
               and type == chr(world_db["MAP"][pos + length - (not ind)])):
            return True
        return False
    store_seed = rand.seed
    rand.seed = world_db["SEED_MAP"]
    world_db["MAP"] = bytearray(b'~' * (world_db["MAP_LENGTH"] ** 2))
    length = world_db["MAP_LENGTH"]
    add_half_width = (not (length % 2)) * int(length / 2)
    world_db["MAP"][int((length ** 2) / 2) + add_half_width] = ord(".")
    while (1):
        y = rand.next() % length
        x = rand.next() % length
        pos = (y * length) + x
        if "~" == chr(world_db["MAP"][pos]) and is_neighbor((y, x), "."):
            if y == 0 or y == (length - 1) or x == 0 or x == (length - 1):
                break
            world_db["MAP"][pos] = ord(".")
    n_trees = int((length ** 2) / 16)
    i_trees = 0
    while (i_trees <= n_trees):
        single_allowed = rand.next() % 32
        y = rand.next() % length
        x = rand.next() % length
        pos = (y * length) + x
        if "." == chr(world_db["MAP"][pos]) \
          and ((not single_allowed) or is_neighbor((y, x), "X")):
            world_db["MAP"][pos] = ord("X")
            i_trees += 1
    rand.seed = store_seed
    # This all-too-precise replica of the original C code misses iter_limit().


def update_map_memory(t):
    """Update t's T_MEMMAP with what's in its FOV now,age its T_MEMMEPTHMAP."""
    if not t["T_MEMMAP"]:
        t["T_MEMMAP"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
    if not t["T_MEMDEPTHMAP"]:
        t["T_MEMDEPTHMAP"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
    for pos in range(world_db["MAP_LENGTH"] ** 2):
        if "v" == chr(t["fovmap"][pos]):
            t["T_MEMDEPTHMAP"][pos] = ord("0")
            if " " == chr(t["T_MEMMAP"][pos]):
                t["T_MEMMAP"][pos] = world_db["MAP"][pos]
            continue
        if ord('0') <= t["T_MEMDEPTHMAP"][pos] \
           and ord('9') >= t["T_MEMDEPTHMAP"][pos] \
           and not rand.next() % (2 ** (t["T_MEMDEPTHMAP"][pos] - 48)):
            t["T_MEMDEPTHMAP"][pos] += 1
    for mt in [mt for mt in t["T_MEMTHING"]
               if "v" == chr(t["fovmap"][(mt[1] * world_db["MAP_LENGTH"])
                                         + mt[2]])]:
            t["T_MEMTHING"].remove(mt)
    for id in world_db["Things"]:
        type = world_db["Things"][id]["T_TYPE"]
        if not world_db["ThingTypes"][type]["TT_LIFEPOINTS"]:
            y = world_db["Things"][id]["T_POSY"]
            x = world_db["Things"][id]["T_POSX"]
            if "v" == chr(t["fovmap"][(y * world_db["MAP_LENGTH"]) + x]):
                t["T_MEMTHING"].append((type, y, x))


def set_world_inactive():
    """Set world_db["WORLD_ACTIVE"] to 0 and remove worldstate file."""
    server_test()
    if os.access(io_db["path_worldstate"], os.F_OK):
        os.remove(io_db["path_worldstate"])
    world_db["WORLD_ACTIVE"] = 0


def integer_test(val_string, min, max):
    """Return val_string if possible integer >= min and <= max, else None."""
    try:
        val = int(val_string)
        if val < min or val > max:
            raise ValueError
        return val
    except ValueError:
        print("Ignoring: Please use integer >= " + str(min) + " and <= " +
              str(max) + ".")
        return None


def setter(category, key, min, max):
    """Build setter for world_db([category + "s"][id])[key] to >=min/<=max."""
    if category is None:
        def f(val_string):
            val = integer_test(val_string, min, max)
            if None != val:
                world_db[key] = val
    else:
        if category == "Thing":
            id_store = command_tid
            decorator = test_Thing_id
        elif category == "ThingType":
            id_store = command_ttid
            decorator = test_ThingType_id
        elif category == "ThingAction":
            id_store = command_taid
            decorator = test_ThingAction_id

        @decorator
        def f(val_string):
            val = integer_test(val_string, min, max)
            if None != val:
                world_db[category + "s"][id_store.id][key] = val
    return f


def build_fov_map(t):
    """Build Thing's FOV map."""
    t["fovmap"] = bytearray(b'v' * (world_db["MAP_LENGTH"] ** 2))
    maptype = ctypes.c_char * len(world_db["MAP"])
    test = libpr.build_fov_map(t["T_POSY"], t["T_POSX"],
                               maptype.from_buffer(t["fovmap"]),
                               maptype.from_buffer(world_db["MAP"]))
    if test:
        raise RuntimeError("Malloc error in build_fov_Map().")


def decrement_lifepoints(t):
    """Decrement t's lifepoints by 1, and if to zero, corpse it.

    If t is the player avatar, only blank its fovmap, so that the client may
    still display memory data. On non-player things, erase fovmap and memory.
    """
    t["T_LIFEPOINTS"] -= 1
    if 0 == t["T_LIFEPOINTS"]:
        t["T_TYPE"] = world_db["ThingTypes"][t["T_TYPE"]]["TT_CORPSE_ID"]
        if world_db["Things"][0] == t:
            t["fovmap"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
            strong_write(io_db["file_out"], "LOG You die.\n")
        else:
            t["fovmap"] = False
            t["T_MEMMAP"] = False
            t["T_MEMDEPTHMAP"] = False
            t["T_MEMTHING"] = []
            strong_write(io_db["file_out"], "LOG It dies.\n")


def mv_yx_in_dir_legal(dir, y, x):
    """Wrapper around libpr.mv_yx_in_dir_legal to simplify its use."""
    dir_c = dir.encode("ascii")[0]
    test = libpr.mv_yx_in_dir_legal_wrap(dir_c, y, x)
    if -1 == test:
        raise RuntimeError("Too much wrapping in mv_yx_in_dir_legal_wrap()!")
    return (test, libpr.result_y(), libpr.result_x())


def actor_wait(t):
    """Make t do nothing (but loudly, if player avatar)."""
    if t == world_db["Things"][0]:
        strong_write(io_db["file_out"], "LOG You wait.\n")


def actor_move(t):
    """If passable, move/collide(=attack) thing into T_ARGUMENT's direction."""
    passable = False
    move_result = mv_yx_in_dir_legal(t["T_ARGUMENT"], t["T_POSY"], t["T_POSX"])
    if 1 == move_result[0]:
        pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
        passable = "." == chr(world_db["MAP"][pos])
        hitted = [id for id in world_db["Things"]
                  if world_db["Things"][id] != t
                  if world_db["Things"][id]["T_LIFEPOINTS"]
                  if world_db["Things"][id]["T_POSY"] == move_result[1]
                  if world_db["Things"][id]["T_POSX"] == move_result[2]]
        if len(hitted):
            hit_id = hitted[0]
            hitter_name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
            hitter = "You" if t == world_db["Things"][0] else hitter_name
            hitted_type = world_db["Things"][hit_id]["T_TYPE"]
            hitted_name = world_db["ThingTypes"][hitted_type]["TT_NAME"]
            hitted = "you" if hit_id == 0 else hitted_name
            verb = " wound " if hitter == "You" else " wounds "
            strong_write(io_db["file_out"], "LOG " + hitter + verb + hitted +
                                            ".\n")
            decrement_lifepoints(world_db["Things"][hit_id])
            return
    dir = [dir for dir in directions_db
           if directions_db[dir] == t["T_ARGUMENT"]][0]
    if passable:
        t["T_POSY"] = move_result[1]
        t["T_POSX"] = move_result[2]
        for id in t["T_CARRIES"]:
            world_db["Things"][id]["T_POSY"] = move_result[1]
            world_db["Things"][id]["T_POSX"] = move_result[2]
        build_fov_map(t)
        strong_write(io_db["file_out"], "LOG You move " + dir + ".\n")
    else:
        strong_write(io_db["file_out"], "LOG You fail to move " + dir + ".\n")


def actor_pick_up(t):
    """Make t pick up (topmost?) Thing from ground into inventory."""
    # Topmostness is actually not defined so far.
    ids = [id for id in world_db["Things"] if world_db["Things"][id] != t
           if not world_db["Things"][id]["carried"]
           if world_db["Things"][id]["T_POSY"] == t["T_POSY"]
           if world_db["Things"][id]["T_POSX"] == t["T_POSX"]]
    if len(ids):
        world_db["Things"][ids[0]]["carried"] = True
        t["T_CARRIES"].append(ids[0])
        if t == world_db["Things"][0]:
            strong_write(io_db["file_out"], "LOG You pick up an object.\n")
    elif t == world_db["Things"][0]:
        err = "You try to pick up an object, but there is none."
        strong_write(io_db["file_out"], "LOG " + err + "\n")


def actor_drop(t):
    """Make t rop Thing from inventory to ground indexed by T_ARGUMENT."""
    # TODO: Handle case where T_ARGUMENT matches nothing.
    if len(t["T_CARRIES"]):
        id = t["T_CARRIES"][t["T_ARGUMENT"]]
        t["T_CARRIES"].remove(id)
        world_db["Things"][id]["carried"] = False
        if t == world_db["Things"][0]:
            strong_write(io_db["file_out"], "LOG You drop an object.\n")
    elif t == world_db["Things"][0]:
        err = "You try to drop an object, but you own none."
        strong_write(io_db["file_out"], "LOG " + err + "\n")


def actor_use(t):
    """Make t use (for now: consume) T_ARGUMENT-indexed Thing in inventory."""
    # TODO: Handle case where T_ARGUMENT matches nothing.
    if len(t["T_CARRIES"]):
        id = t["T_CARRIES"][t["T_ARGUMENT"]]
        type = world_db["Things"][id]["T_TYPE"]
        if world_db["ThingTypes"][type]["TT_CONSUMABLE"]:
            t["T_CARRIES"].remove(id)
            del world_db["Things"][id]
            t["T_SATIATION"] += world_db["ThingTypes"][type]["TT_CONSUMABLE"]
            t["T_LIFEPOINTS"] += 1
            # Wrongly increment HPs is a replica of the original code.
            strong_write(io_db["file_out"], "LOG You consume this object.\n")
        else:
            strong_write(io_db["file_out"], "LOG You try to use this object," +
                                            "but fail.\n")
    else:
        strong_write(io_db["file_out"], "LOG You try to use an object, but " +
                                        "you own none.\n")


def thingproliferation(t):
    """To chance of 1/TT_PROLIFERATE, create t offspring in neighbor cell.

    Naturally only works with TT_PROLIFERATE > 0. The neighbor cell must be
    passable and not be inhabited by a Thing of the same type, or, if Thing is
    animate, any other animate Thing. If there are several map cell candidates,
    one is selected randomly.
    """
    def test_cell(t, y, x):
        if "." == chr(world_db["MAP"][(y * world_db["MAP_LENGTH"]) + x]):
            for id in [id for id in world_db["Things"]
                       if y == world_db["Things"][id]["T_POSY"]
                       if x == world_db["Things"][id]["T_POSX"]
                       if (t["T_TYPE"] == world_db["Things"][id]["T_TYPE"])
                       or (t["T_LIFEPOINTS"] and
                           world_db["Things"][id]["T_LIFEPOINTS"])]:
                return False
            return True
        return False
    prolscore = world_db["ThingTypes"][t["T_TYPE"]]["TT_PROLIFERATE"]
    if prolscore and (1 == prolscore or 1 == (rand.next() % prolscore)):
        candidates = []
        for dir in [directions_db[key] for key in directions_db]:
            mv_result = mv_yx_in_dir_legal(dir, t["T_POSY"], t["T_POSX"])
            if mv_result[0] and test_cell(t, mv_result[1], mv_result[2]):
                candidates.append((mv_result[1], mv_result[2]))
        if len(candidates):
            i = rand.next() % len(candidates)
            id = id_setter(-1, "Things")
            newT = new_Thing(t["T_TYPE"], (candidates[i][0], candidates[i][1]))
            world_db["Things"][id] = newT


def try_healing(t):
    """Grow t's HP to a 1/32 chance if < HP max, satiation > 0, and waiting.

    On success, decrease satiation score by 32.
    """
    if t["T_SATIATION"] > 0 \
       and t["T_LIFEPOINTS"] < \
           world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"] \
       and 0 == (rand.next() % 31) \
       and t["T_COMMAND"] == [id for id in world_db["ThingActions"]
                              if world_db["ThingActions"][id]["TA_NAME"] ==
                                 "wait"][0]:
        t["T_LIFEPOINTS"] += 1
        t["T_SATIATION"] -= 32
        if t == world_db["Things"][0]:
            strong_write(io_db["file_out"], "LOG You heal.\n")
        else:
            name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
            strong_write(io_db["file_out"], "LOG " + name + "heals.\n")


def hunger(t):
    """Decrement t's satiation,dependent on it trigger lifepoint dec chance."""
    if t["T_SATIATION"] > -32768:
        t["T_SATIATION"] -= 1
    testbase = t["T_SATIATION"] if t["T_SATIATION"] >= 0 else -t["T_SATIATION"]
    if not world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"]:
        raise RuntimeError("A thing that should not hunger is hungering.")
    stomach = int(32767 / world_db["ThingTypes"][t["T_TYPE"]]["TT_LIFEPOINTS"])
    if int(int(testbase / stomach) / ((rand.next() % stomach) + 1)):
        if t == world_db["Things"][0]:
            strong_write(io_db["file_out"], "LOG You suffer from hunger.\n")
        else:
            name = world_db["ThingTypes"][t["T_TYPE"]]["TT_NAME"]
            strong_write(io_db["file_out"], "LOG " + name +
                                            " suffers from hunger.\n")
        decrement_lifepoints(t)


def get_dir_to_nearest_target(t, c):
    # Dummy
    return False


def standing_on_consumable(t):
    """Return True/False whether t is standing on a consumable."""
    for id in [id for id in world_db["Things"] if world_db["Things"][id] != t
               if world_db["Things"][id]["T_POSY"] == t["T_POSY"]
               if world_db["Things"][id]["T_POSX"] == t["T_POSX"]
               if world_db["ThingTypes"][world_db["Things"][id]["T_TYPE"]]
                          ["TT_CONSUMABLE"]]:
        return True
    return False


def get_inventory_slot_to_consume(t):
    """Return slot Id of strongest consumable in t's inventory, else -1."""
    cmp_consumability = 0
    selection = -1
    i = 0
    for id in t["T_CARRIES"]:
        type = world_db["Things"][id]["T_TYPE"]
        if world_db["ThingTypes"][type]["TT_CONSUMABLE"] > cmp_consumability:
            cmp_consumability = world_db["ThingTypes"][type]["TT_CONSUMABLE"]
            selection = i
        i += 1
    return selection


def ai(t):
    """Determine next command/argment for actor t via AI algorithms.

    AI will look for, and move towards, enemies (animate Things not of their
    own ThingType); if they see none, they will consume consumables in their
    inventory; if there are none, they will pick up what they stand on if they
    stand on consumables; if they stand on none, they will move towards the
    next consumable they see or remember on the map; if they see or remember
    none, they will explore parts of the map unseen since ever or for at least
    one turn; if there is nothing to explore, they will simply wait.
    """
    t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                      if world_db["ThingActions"][id]["TA_NAME"] == "wait"][0]
    if not get_dir_to_nearest_target(t, "f"):
        sel = get_inventory_slot_to_consume(t)
        if -1 != sel:
            t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                              if world_db["ThingActions"][id]["TA_NAME"]
                                 == "use"][0]
            t["T_ARGUMENT"] = sel
        elif standing_on_consumable(t):
            t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                              if world_db["ThingActions"][id]["TA_NAME"]
                                 == "pick_up"][0]
        elif (not get_dir_to_nearest_target(t, "c")) and \
             (not get_dir_to_nearest_target(t, "a")):
            get_dir_to_nearest_target(t, "s")


def turn_over():
    """Run game world and its inhabitants until new player input expected."""
    id = 0
    whilebreaker = False
    while world_db["Things"][0]["T_LIFEPOINTS"]:
        for id in [id for id in world_db["Things"]]:
            if not id in world_db["Things"]: # Thing may have been consumed
                continue                     # during turn …
            Thing = world_db["Things"][id]
            if Thing["T_LIFEPOINTS"]:
                if not Thing["T_COMMAND"]:
                    update_map_memory(Thing)
                    if 0 == id:
                        whilebreaker = True
                        break
                    ai(Thing)
                    Thing["T_COMMAND"] = 1
                try_healing(Thing)
                Thing["T_PROGRESS"] += 1
                taid = [a for a in world_db["ThingActions"]
                          if a == Thing["T_COMMAND"]][0]
                ThingAction = world_db["ThingActions"][taid]
                if Thing["T_PROGRESS"] == ThingAction["TA_EFFORT"]:
                    eval("actor_" + ThingAction["TA_NAME"])(Thing)
                    Thing["T_COMMAND"] = 0
                    Thing["T_PROGRESS"] = 0
                hunger(Thing)
            thingproliferation(Thing)
        if whilebreaker:
            break
        world_db["TURN"] += 1


def new_Thing(type, pos=(0, 0)):
    """Return Thing of type T_TYPE, with fovmap if alive and world active."""
    thing = {
        "T_LIFEPOINTS": world_db["ThingTypes"][type]["TT_LIFEPOINTS"],
        "T_ARGUMENT": 0,
        "T_PROGRESS": 0,
        "T_SATIATION": 0,
        "T_COMMAND": 0,
        "T_TYPE": type,
        "T_POSY": pos[0],
        "T_POSX": pos[1],
        "T_CARRIES": [],
        "carried": False,
        "T_MEMTHING": [],
        "T_MEMMAP": False,
        "T_MEMDEPTHMAP": False,
        "fovmap": False
    }
    if world_db["WORLD_ACTIVE"] and thing["T_LIFEPOINTS"]:
        build_fov_map(thing)
    return thing


def id_setter(id, category, id_store=False, start_at_1=False):
    """Set ID of object of category to manipulate ID unused? Create new one.

    The ID is stored as id_store.id (if id_store is set). If the integer of the
    input is valid (if start_at_1, >= 0 and <= 255, else >= -32768 and <=
    32767), but <0 or (if start_at_1) <1, calculate new ID: lowest unused ID
    >=0 or (if start_at_1) >= 1, and <= 255. None is always returned when no
    new object is created, otherwise the new object's ID.
    """
    min = 0 if start_at_1 else -32768
    max = 255 if start_at_1 else 32767
    if str == type(id):
        id = integer_test(id, min, max)
    if None != id:
        if id in world_db[category]:
            if id_store:
                id_store.id = id
            return None
        else:
            if (start_at_1 and 0 == id) \
               or ((not start_at_1) and (id < 0 or id > 255)):
                id = -1
                while 1:
                    id = id + 1
                    if id not in world_db[category]:
                        break
                if id > 255:
                    print("Ignoring: "
                          "No unused ID available to add to ID list.")
                    return None
            if id_store:
                id_store.id = id
    return id


def command_ping():
    """Send PONG line to server output file."""
    strong_write(io_db["file_out"], "PONG\n")


def command_quit():
    """Abort server process."""
    raise SystemExit("received QUIT command")


def command_thingshere(str_y, str_x):
    """Write to out file list of Things known to player at coordinate y, x."""
    def write_thing_if_here():
        if y == world_db["Things"][id]["T_POSY"] \
           and x == world_db["Things"][id]["T_POSX"] \
           and not world_db["Things"][id]["carried"]:
            type = world_db["Things"][id]["T_TYPE"]
            name = world_db["ThingTypes"][type]["TT_NAME"]
            strong_write(io_db["file_out"], name + "\n")
    if world_db["WORLD_ACTIVE"]:
        y = integer_test(str_y, 0, 255)
        x = integer_test(str_x, 0, 255)
        length = world_db["MAP_LENGTH"]
        if None != y and None != x and y < length and x < length:
            pos = (y * world_db["MAP_LENGTH"]) + x
            strong_write(io_db["file_out"], "THINGS_HERE START\n")
            if "v" == chr(world_db["Things"][0]["fovmap"][pos]):
                for id in world_db["Things"]:
                    write_thing_if_here()
            else:
                for id in world_db["Things"][0]["T_MEMTHING"]:
                    write_thing_if_here()
            strong_write(io_db["file_out"], "THINGS_HERE END\n")
        else:
            print("Ignoring: Invalid map coordinates.")
    else:
        print("Ignoring: Command only works on existing worlds.")


def play_commander(action, args=False):
    """Setter for player's T_COMMAND and T_ARGUMENT, then calling turn_over().

    T_ARGUMENT is set to direction char if action=="wait",or 8-bit int if args.
    """

    def set_command():
        id = [x for x in world_db["ThingActions"]
                if world_db["ThingActions"][x]["TA_NAME"] == action][0]
        world_db["Things"][0]["T_COMMAND"] = id
        turn_over()

    def set_command_and_argument_int(str_arg):
        val = integer_test(str_arg, 0, 255)
        if None != val:
            world_db["Things"][0]["T_ARGUMENT"] = val
            set_command()

    def set_command_and_argument_movestring(str_arg):
        if str_arg in directions_db:
            world_db["Things"][0]["T_ARGUMENT"] = directions_db[str_arg]
            set_command()
        else:
            print("Ignoring: Argument must be valid direction string.")

    if action == "move":
        return set_command_and_argument_movestring
    elif args:
        return set_command_and_argument_int
    else:
        return set_command


def command_seedrandomness(seed_string):
    """Set rand seed to int(seed_string)."""
    val = integer_test(seed_string, 0, 4294967295)
    if None != val:
        rand.seed = val


def command_seedmap(seed_string):
    """Set world_db["SEED_MAP"] to int(seed_string), then (re-)make map."""
    setter(None, "SEED_MAP", 0, 4294967295)(seed_string)
    remake_map()


def command_makeworld(seed_string):
    """(Re-)build game world, i.e. map, things, to a new turn 1 from seed.

    Seed rand with seed, fill it into world_db["SEED_MAP"]. Do more only with a
    "wait" ThingAction and world["PLAYER_TYPE"] matching ThingType of
    TT_START_NUMBER > 0. Then, world_db["Things"] emptied, call remake_map()
    and set world_db["WORLD_ACTIVE"], world_db["TURN"] to 1. Build new Things
    according to ThingTypes' TT_START_NUMBERS, with Thing of ID 0 to ThingType
    of ID = world["PLAYER_TYPE"]. Place Things randomly, and actors not on each
    other. Init player's memory map. Write "NEW_WORLD" line to out file.
    """

    def free_pos():
        i = 0
        while 1:
            err = "Space to put thing on too hard to find. Map too small?"
            while 1:
                y = rand.next() % world_db["MAP_LENGTH"]
                x = rand.next() % world_db["MAP_LENGTH"]
                if "." == chr(world_db["MAP"][y * world_db["MAP_LENGTH"] + x]):
                    break
                i += 1
                if i == 65535:
                    raise SystemExit(err)
            # Replica of C code, wrongly ignores animatedness of new Thing.
            pos_clear = (0 == len([id for id in world_db["Things"]
                                   if world_db["Things"][id]["T_LIFEPOINTS"]
                                   if world_db["Things"][id]["T_POSY"] == y
                                   if world_db["Things"][id]["T_POSX"] == x]))
            if pos_clear:
                break
        return (y, x)

    val = integer_test(seed_string, 0, 4294967295)
    if None == val:
        return
    rand.seed = val
    world_db["SEED_MAP"] = val
    player_will_be_generated = False
    playertype = world_db["PLAYER_TYPE"]
    for ThingType in world_db["ThingTypes"]:
        if playertype == ThingType:
            if 0 < world_db["ThingTypes"][ThingType]["TT_START_NUMBER"]:
                player_will_be_generated = True
            break
    if not player_will_be_generated:
        print("Ignoring beyond SEED_MAP: " +
              "No player type with start number >0 defined.")
        return
    wait_action = False
    for ThingAction in world_db["ThingActions"]:
        if "wait" == world_db["ThingActions"][ThingAction]["TA_NAME"]:
            wait_action = True
    if not wait_action:
        print("Ignoring beyond SEED_MAP: " +
              "No thing action with name 'wait' defined.")
        return
    world_db["Things"] = {}
    remake_map()
    world_db["WORLD_ACTIVE"] = 1
    world_db["TURN"] = 1
    for i in range(world_db["ThingTypes"][playertype]["TT_START_NUMBER"]):
        id = id_setter(-1, "Things")
        world_db["Things"][id] = new_Thing(playertype, free_pos())
    update_map_memory(world_db["Things"][0])
    for type in world_db["ThingTypes"]:
        for i in range(world_db["ThingTypes"][type]["TT_START_NUMBER"]):
            if type != playertype:
                id = id_setter(-1, "Things")
                world_db["Things"][id] = new_Thing(type, free_pos())
    strong_write(io_db["file_out"], "NEW_WORLD\n")


def command_maplength(maplength_string):
    """Redefine map length. Invalidate map, therefore lose all things on it."""
    val = integer_test(maplength_string, 1, 256)
    if None != val:
        world_db["MAP_LENGTH"] = val
        set_world_inactive()
        world_db["Things"] = {}
        libpr.set_maplength(val)


def command_worldactive(worldactive_string):
    """Toggle world_db["WORLD_ACTIVE"] if possible.

    An active world can always be set inactive. An inactive world can only be
    set active with a "wait" ThingAction, and a player Thing (of ID 0). On
    activation, rebuild all Things' FOVs, and the player's map memory.
    """
    # In original version, map existence was also tested (unnecessarily?).
    val = integer_test(worldactive_string, 0, 1)
    if val:
        if 0 != world_db["WORLD_ACTIVE"]:
            if 0 == val:
                set_world_inactive()
            else:
                print("World already active.")
        elif 0 == world_db["WORLD_ACTIVE"]:
            wait_exists = False
            for ThingAction in world_db["ThingActions"]:
                if "wait" == world_db["ThingActions"][ThingAction]["TA_NAME"]:
                    wait_exists = True
                    break
            player_exists = False
            for Thing in world_db["Things"]:
                if 0 == Thing:
                    player_exists = True
                    break
            if wait_exists and player_exists:
                for id in world_db["Things"]:
                    if world_db["Things"][id]["T_LIFEPOINTS"]:
                        build_fov_map(world_db["Things"][id])
                        if 0 == id:
                            update_map_memory(world_db["Things"][id])
                world_db["WORLD_ACTIVE"] = 1


def test_for_id_maker(object, category):
    """Return decorator testing for object having "id" attribute."""
    def decorator(f):
        def helper(*args):
            if hasattr(object, "id"):
                f(*args)
            else:
                print("Ignoring: No " + category +
                      " defined to manipulate yet.")
        return helper
    return decorator


def command_tid(id_string):
    """Set ID of Thing to manipulate. ID unused? Create new one.

    Default new Thing's type to the first available ThingType, others: zero.
    """
    id = id_setter(id_string, "Things", command_tid)
    if None != id:
        if world_db["ThingTypes"] == {}:
            print("Ignoring: No ThingType to settle new Thing in.")
            return
        type = list(world_db["ThingTypes"].keys())[0]
        world_db["Things"][id] = new_Thing(type)


test_Thing_id = test_for_id_maker(command_tid, "Thing")


@test_Thing_id
def command_tcommand(str_int):
    """Set T_COMMAND of selected Thing."""
    val = integer_test(str_int, 0, 255)
    if None != val:
        if 0 == val or val in world_db["ThingActions"]:
            world_db["Things"][command_tid.id]["T_COMMAND"] = val
        else:
            print("Ignoring: ThingAction ID belongs to no known ThingAction.")


@test_Thing_id
def command_ttype(str_int):
    """Set T_TYPE of selected Thing."""
    val = integer_test(str_int, 0, 255)
    if None != val:
        if val in world_db["ThingTypes"]:
            world_db["Things"][command_tid.id]["T_TYPE"] = val
        else:
            print("Ignoring: ThingType ID belongs to no known ThingType.")


@test_Thing_id
def command_tcarries(str_int):
    """Append int(str_int) to T_CARRIES of selected Thing.

    The ID int(str_int) must not be of the selected Thing, and must belong to a
    Thing with unset "carried" flag. Its "carried" flag will be set on owning.
    """
    val = integer_test(str_int, 0, 255)
    if None != val:
        if val == command_tid.id:
            print("Ignoring: Thing cannot carry itself.")
        elif val in world_db["Things"] \
             and not world_db["Things"][val]["carried"]:
            world_db["Things"][command_tid.id]["T_CARRIES"].append(val)
            world_db["Things"][val]["carried"] = True
        else:
            print("Ignoring: Thing not available for carrying.")
    # Note that the whole carrying structure is different from the C version:
    # Carried-ness is marked by a "carried" flag, not by Things containing
    # Things internally.


@test_Thing_id
def command_tmemthing(str_t, str_y, str_x):
    """Add (int(str_t), int(str_y), int(str_x)) to selected Thing's T_MEMTHING.

    The type must fit to an existing ThingType, and the position into the map.
    """
    type = integer_test(str_t, 0, 255)
    posy = integer_test(str_y, 0, 255)
    posx = integer_test(str_x, 0, 255)
    if None != type and None != posy and None != posx:
        if type not in world_db["ThingTypes"] \
           or posy >= world_db["MAP_LENGTH"] or posx >= world_db["MAP_LENGTH"]:
            print("Ignoring: Illegal value for thing type or position.")
        else:
            memthing = (type, posy, posx)
            world_db["Things"][command_tid.id]["T_MEMTHING"].append(memthing)


def setter_map(maptype):
    """Set selected Thing's map of maptype's int(str_int)-th line to mapline.

    If Thing has no map of maptype yet, initialize it with ' ' bytes first.
    """
    @test_Thing_id
    def helper(str_int, mapline):
        val = integer_test(str_int, 0, 255)
        if None != val:
            if val >= world_db["MAP_LENGTH"]:
                print("Illegal value for map line number.")
            elif len(mapline) != world_db["MAP_LENGTH"]:
                print("Map line length is unequal map width.")
            else:
                length = world_db["MAP_LENGTH"]
                map = None
                if not world_db["Things"][command_tid.id][maptype]:
                    map = bytearray(b' ' * (length ** 2))
                else:
                    map = world_db["Things"][command_tid.id][maptype]
                map[val * length:(val * length) + length] = mapline.encode()
                world_db["Things"][command_tid.id][maptype] = map
    return helper


def setter_tpos(axis):
    """Generate setter for T_POSX or  T_POSY of selected Thing.

    If world is active, rebuilds animate things' fovmap, player's memory map.
    """
    @test_Thing_id
    def helper(str_int):
        val = integer_test(str_int, 0, 255)
        if None != val:
            if val < world_db["MAP_LENGTH"]:
                world_db["Things"][command_tid.id]["T_POS" + axis] = val
                if world_db["WORLD_ACTIVE"] \
                   and world_db["Things"][command_tid.id]["T_LIFEPOINTS"]:
                    build_fov_map(world_db["Things"][command_tid.id])
                    if 0 == command_tid.id:
                        update_map_memory(world_db["Things"][command_tid.id])
            else:
                print("Ignoring: Position is outside of map.")
    return helper


def command_ttid(id_string):
    """Set ID of ThingType to manipulate. ID unused? Create new one.

    Default new ThingType's TT_SYMBOL to "?", TT_CORPSE_ID to self, others: 0.
    """
    id = id_setter(id_string, "ThingTypes", command_ttid)
    if None != id:
        world_db["ThingTypes"][id] = {
            "TT_NAME": "(none)",
            "TT_CONSUMABLE": 0,
            "TT_LIFEPOINTS": 0,
            "TT_PROLIFERATE": 0,
            "TT_START_NUMBER": 0,
            "TT_SYMBOL": "?",
            "TT_CORPSE_ID": id
        }


test_ThingType_id = test_for_id_maker(command_ttid, "ThingType")


@test_ThingType_id
def command_ttname(name):
    """Set TT_NAME of selected ThingType."""
    world_db["ThingTypes"][command_ttid.id]["TT_NAME"] = name


@test_ThingType_id
def command_ttsymbol(char):
    """Set TT_SYMBOL of selected ThingType. """
    if 1 == len(char):
        world_db["ThingTypes"][command_ttid.id]["TT_SYMBOL"] = char
    else:
        print("Ignoring: Argument must be single character.")


@test_ThingType_id
def command_ttcorpseid(str_int):
    """Set TT_CORPSE_ID of selected ThingType."""
    val = integer_test(str_int, 0, 255)
    if None != val:
        if val in world_db["ThingTypes"]:
            world_db["ThingTypes"][command_ttid.id]["TT_CORPSE_ID"] = val
        else:
            print("Ignoring: Corpse ID belongs to no known ThignType.")


def command_taid(id_string):
    """Set ID of ThingAction to manipulate. ID unused? Create new one.

    Default new ThingAction's TA_EFFORT to 1, its TA_NAME to "wait".
    """
    id = id_setter(id_string, "ThingActions", command_taid, True)
    if None != id:
        world_db["ThingActions"][id] = {
            "TA_EFFORT": 1,
            "TA_NAME": "wait"
        }


test_ThingAction_id = test_for_id_maker(command_taid, "ThingAction")


@test_ThingAction_id
def command_taname(name):
    """Set TA_NAME of selected ThingAction.

    The name must match a valid thing action function. If after the name
    setting no ThingAction with name "wait" remains, call set_world_inactive().
    """
    if name == "wait" or name == "move" or name == "use" or name == "drop" \
       or name == "pick_up":
        world_db["ThingActions"][command_taid.id]["TA_NAME"] = name
        if 1 == world_db["WORLD_ACTIVE"]:
            wait_defined = False
            for id in world_db["ThingActions"]:
                if "wait" == world_db["ThingActions"][id]["TA_NAME"]:
                    wait_defined = True
                    break
            if not wait_defined:
                set_world_inactive()
    else:
        print("Ignoring: Invalid action name.")
    # In contrast to the original,naming won't map a function to a ThingAction.


def command_ai():
    """Call ai() on player Thing, then turn_over()."""
    ai(world_db["Things"][0])
    turn_over()


"""Commands database.

Map command start tokens to ([0]) number of expected command arguments, ([1])
the command's meta-ness (i.e. is it to be written to the record file, is it to
be ignored in replay mode if read from server input file), and ([2]) a function
to be called on it.
"""
commands_db = {
    "QUIT": (0, True, command_quit),
    "PING": (0, True, command_ping),
    "THINGS_HERE": (2, True, command_thingshere),
    "MAKE_WORLD": (1, False, command_makeworld),
    "SEED_MAP": (1, False, command_seedmap),
    "SEED_RANDOMNESS": (1, False, command_seedrandomness),
    "TURN": (1, False, setter(None, "TURN", 0, 65535)),
    "PLAYER_TYPE": (1, False, setter(None, "PLAYER_TYPE", 0, 255)),
    "MAP_LENGTH": (1, False, command_maplength),
    "WORLD_ACTIVE": (1, False, command_worldactive),
    "TA_ID": (1, False, command_taid),
    "TA_EFFORT": (1, False, setter("ThingAction", "TA_EFFORT", 0, 255)),
    "TA_NAME": (1, False, command_taname),
    "TT_ID": (1, False, command_ttid),
    "TT_NAME": (1, False, command_ttname),
    "TT_SYMBOL": (1, False, command_ttsymbol),
    "TT_CORPSE_ID": (1, False, command_ttcorpseid),
    "TT_CONSUMABLE": (1, False, setter("ThingType", "TT_CONSUMABLE",
                                       0, 65535)),
    "TT_START_NUMBER": (1, False, setter("ThingType", "TT_START_NUMBER",
                                         0, 255)),
    "TT_PROLIFERATE": (1, False, setter("ThingType", "TT_PROLIFERATE",
                                        0, 255)),
    "TT_LIFEPOINTS": (1, False, setter("ThingType", "TT_LIFEPOINTS", 0, 255)),
    "T_ID": (1, False, command_tid),
    "T_ARGUMENT": (1, False, setter("Thing", "T_ARGUMENT", 0, 255)),
    "T_PROGRESS": (1, False, setter("Thing", "T_PROGRESS", 0, 255)),
    "T_LIFEPOINTS": (1, False, setter("Thing", "T_LIFEPOINTS", 0, 255)),
    "T_SATIATION": (1, False, setter("Thing", "T_SATIATION", -32768, 32767)),
    "T_COMMAND": (1, False, command_tcommand),
    "T_TYPE": (1, False, command_ttype),
    "T_CARRIES": (1, False, command_tcarries),
    "T_MEMMAP": (2, False, setter_map("T_MEMMAP")),
    "T_MEMDEPTHMAP": (2, False, setter_map("T_MEMDEPTHMAP")),
    "T_MEMTHING": (3, False, command_tmemthing),
    "T_POSY": (1, False, setter_tpos("Y")),
    "T_POSX": (1, False, setter_tpos("X")),
    "wait": (0, False, play_commander("wait")),
    "move": (1, False, play_commander("move")),
    "pick_up": (0, False, play_commander("pick_up")),
    "drop": (1, False, play_commander("drop", True)),
    "use": (1, False, play_commander("use", True)),
    "ai": (0, False, command_ai)
}


"""World state database. With sane default values. (Randomness is in rand.)"""
world_db = {
    "TURN": 0,
    "SEED_MAP": 0,
    "PLAYER_TYPE": 0,
    "MAP_LENGTH": 64,
    "WORLD_ACTIVE": 0,
    "ThingActions": {},
    "ThingTypes": {},
    "Things": {}
}

"""Mapping of direction names to internal direction chars."""
directions_db = {"east": "d", "south-east": "c", "south-west": "x",
                 "west": "s", "north-west": "w", "north-east": "e"}

"""File IO database."""
io_db = {
    "path_save": "save",
    "path_record": "record",
    "path_worldconf": "confserver/world",
    "path_server": "server/",
    "path_in": "server/in",
    "path_out": "server/out",
    "path_worldstate": "server/worldstate",
    "tmp_suffix": "_tmp",
    "kicked_by_rival": False,
    "worldstate_updateable": False
}


try:
    libpr = prep_library()
    rand = RandomnessIO()
    opts = parse_command_line_arguments()
    setup_server_io()
    if None != opts.replay:
        replay_game()
    else:
        play_game()
except SystemExit as exit:
    print("ABORTING: " + exit.args[0])
except:
    print("SOMETHING WENT WRONG IN UNEXPECTED WAYS")
    raise
finally:
    cleanup_server_io()
