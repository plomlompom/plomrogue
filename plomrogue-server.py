import argparse
import errno
import os
import shlex
import shutil
import time


def strong_write(file, string):
    """Apply write(string), flush() and os.fsync() to file."""
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
                rmap = world_db["Things"][id][key]
                length = world_db["MAP_LENGTH"]
                for i in range(length):
                    line = rmap[i * length:(i * length) + length].decode()
                    string = string + key + " " + str(i) + quote(line) + "\n"
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
                              "T_MEMTHING": memthing,
                              "T_MEMDEPTHMAP": mapsetter("T_MEMDEPTHMAP")})
    for id in world_db["Things"]:
        if [] != world_db["Things"][id]["T_CARRIES"]:
            string = string + "T_ID " + str(id) + "\n"
            for carried_id in world_db["Things"][id]["T_CARRIES"]:
                string = string + "T_CARRIES " + str(carried_id) + "\n"
    string = string + "WORLD_ACTIVE " + str(world_db["WORLD_ACTIVE"])
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
    wait_on_fail = 1
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
        for i in range(length):
            line = world_db["MAP"][i * length:(i * length) + length].decode()
            string = string + line + "\n"
        # TODO: no proper user-subjective map
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
    # DUMMY map creator.
    world_db["MAP"] = bytearray(b'.' * (world_db["MAP_LENGTH"] ** 2))


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


def command_thingshere(y, x):
    # DUMMY
    print("Ignoring not-yet implemented THINGS_HERE command.")


def command_seedmap(seed_string):
    """Set world_db["SEED_MAP"] to int(seed_string), then (re-)make map."""
    setter(None, "SEED_MAP", 0, 4294967295)(seed_string)
    remake_map()


def command_makeworld(seed_string):
    # DUMMY.
    setter(None, "SEED_RANDOMNESS", 0, 4294967295)(seed_string)
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
    setter(None, "SEED_MAP", 0, 4294967295)(seed_string)
    world_db["Things"] = {}
    remake_map()
    world_db["WORLD_ACTIVE"] = 1
    world_db["TURN"] = 1
    for i in range(world_db["ThingTypes"][playertype]["TT_START_NUMBER"]):
        world_db["Things"][id_setter(-1, "Things")] = {
            "T_LIFEPOINTS": world_db["ThingTypes"][playertype]["TT_LIFEPOINTS"],
            "T_TYPE": playertype,
            "T_POSY": 0, # randomize safely
            "T_POSX": 0, # randomize safely
            "T_ARGUMENT": 0,
            "T_PROGRESS": 0,
            "T_SATIATION": 0,
            "T_COMMAND": 0,
            "T_CARRIES": [],
            "carried": False,
            "T_MEMTHING": [],
            "T_MEMMAP": False,
            "T_MEMDEPTHMAP": False
        }
    # generate fov map?
    # TODO: Generate things (player first, with updated memory)
    strong_write(io_db["file_out"], "NEW_WORLD\n")


def command_maplength(maplength_string):
    """Redefine map length. Invalidate map, therefore lose all things on it."""
    set_world_inactive()
    world_db["Things"] = {}
    setter(None, "MAP_LENGTH", 1, 256)(maplength_string)


def command_worldactive(worldactive_string):
    """Toggle world_db["WORLD_ACTIVE"] if possible.

    An active world can always be set inactive. An inactive world can only be
    set active with a "wait" ThingAction, and a player Thing (of ID 0). On
    activation, rebuild all Things' FOVs and map memories.
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
                # TODO: rebuild all things' FOVs, map memories
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
        world_db["Things"][id] = {
            "T_LIFEPOINTS": 0,
            "T_ARGUMENT": 0,
            "T_PROGRESS": 0,
            "T_SATIATION": 0,
            "T_COMMAND": 0,
            "T_TYPE": list(world_db["ThingTypes"].keys())[0],
            "T_POSY": 0,
            "T_POSX": 0,
            "T_CARRIES": [],
            "carried": False,
            "T_MEMTHING": [],
            "T_MEMMAP": False,
            "T_MEMDEPTHMAP": False
        }


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
                rmap = None
                if not world_db["Things"][command_tid.id][maptype]:
                    rmap = bytearray(b' ' * (length ** 2))
                else:
                    rmap = world_db["Things"][command_tid.id][maptype]
                rmap[val * length:(val * length) + length] = mapline.encode()
                world_db["Things"][command_tid.id][maptype] = rmap
    return helper


def setter_tpos(axis):
    """Generate setter for T_POSX or  T_POSY of selected Thing."""
    @test_Thing_id
    def helper(str_int):
        val = integer_test(str_int, 0, 255)
        if None != val:
            if val < world_db["MAP_LENGTH"]:
                world_db["Things"][command_tid.id]["T_POS" + axis] = val
                # TODO: Delete Thing's FOV, and rebuild it if world is active.
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
    "SEED_RANDOMNESS": (1, False, setter(None, "SEED_RANDOMNESS",
                                         0, 4294967295)),
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
}


"""World state database. With sane default values."""
world_db = {
    "TURN": 0,
    "SEED_MAP": 0,
    "SEED_RANDOMNESS": 0,
    "PLAYER_TYPE": 0,
    "MAP_LENGTH": 64,
    "WORLD_ACTIVE": 0,
    "ThingActions": {},
    "ThingTypes": {},
    "Things": {}
}


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
    opts = parse_command_line_arguments()
    setup_server_io()
    # print("DUMMY: Run game.")
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
    # print("DUMMY: (Clean up C heap.)")
