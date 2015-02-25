import argparse
import errno
import os
import shlex
import shutil
import time


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
    io_db["file_out"].write(io_db["teststring"] + "\n")
    io_db["file_out"].flush()
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

    The command string is tokenized by shlex.split(comments=True). If replay is
    set, a non-meta command from the commands_db merely triggers obey() on the
    next command from the records file. If not, and do do_record is set,
    non-meta commands are recorded via record(), and save_world() is called.
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
            commands_db[tokens[0]][2]()
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
    else:
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
    file.write(text)
    file.flush()
    os.fsync(file.fileno())
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
    # Dummy for saving all commands to reconstruct current world state.
    # Misses same optimizations as record() from the original record().
    ta_string = ""
    for id in world_db["ThingActions"]:
        ta = world_db["ThingActions"][id]
        ta_string = ta_string + "TA_ID " + str(id) + "\n" + \
                                "TA_EFFORT " + str(ta["TA_EFFORT"]) + "\n"
    atomic_write(io_db["path_save"],
                 "WORLD_ACTIVE " + str(world_db["WORLD_ACTIVE"]) + "\n" +
                 "MAP_LENGTH " + str(world_db["MAP_LENGTH"]) + "\n" +
                 "PLAYER_TYPE " + str(world_db["PLAYER_TYPE"]) + "\n" +
                 "TURN " + str(world_db["TURN"]) + "\n" +
                 "SEED_RANDOMNESS " + str(world_db["SEED_RANDOMNESS"]) + "\n" +
                 "SEED_MAP " + str(world_db["SEED_MAP"]) + "\n" +
                 ta_string)
    # TODO: If all this ever does is just writing down what's in world_db, some
    # loop over its entries should be all that's needed.


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


def replay_game():
    """Replay game from record file.

    Use opts.replay as breakpoint turn to which to replay automatically before
    switching to manual input by non-meta commands in server input file
    triggering further reads of record file. Ensure opts.replay is at least 1.
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
        obey(read_command(), "in file", replay=True)


def play_game():
    """Play game by server input file commands. Before, load save file found.

    If no save file is found, a new world is generated from the commands in the
    world config plus a 'MAKE WORLD [current Unix timestamp]'. Record this
    command and all that follow via the server input file.
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
        obey(read_command(), "in file", do_record=True)


def remake_map():
    # DUMMY.
    print("I'd (re-)make the map now, if only I knew how.")


def set_world_inactive():
    """Set world_db["WORLD_ACTIVE"] to 0 and remove worldstate file."""
    server_test()
    if os.access(io_db["path_worldstate"], os.F_OK):
        os.remove(io_db["path_worldstate"])
    world_db["WORLD_ACTIVE"] = 0


def integer_test(val_string, min, max):
    """Return val_string if possible integer >= min and <= max, else False."""
    try:
        val = int(val_string)
        if val < min or val > max:
            raise ValueError
        return val
    except ValueError:
        print("Ignoring: Please use integer >= " + str(min) + " and <= " +
              str(max) + ".")
        return False


def worlddb_value_setter(key, min, max):
    """Generate: Set world_db[key] to int(val_string) if >= min and <= max."""
    def func(val_string):
        val = integer_test(val_string, min, max)
        if val:
            world_db[key] = val
    return func


def command_ping():
    """Send PONG line to server output file."""
    io_db["file_out"].write("PONG\n")
    io_db["file_out"].flush()


def command_quit():
    """Abort server process."""
    raise SystemExit("received QUIT command")


def command_seedmap(seed_string):
    """Set world_db["SEED_MAP"] to int(seed_string), then (re-)make map."""
    worlddb_value_setter("SEED_MAP", 0, 4294967295)(seed_string)
    remake_map()


def command_makeworld(seed_string):
    # DUMMY.
    worlddb_value_setter("SEED_MAP", 0, 4294967295)(seed_string)
    worlddb_value_setter("SEED_RANDOMNESS", 0, 4294967295)(seed_string)
    # TODO: Test for existence of player thing and 'wait' thing action?


def command_maplength(maplength_string):
    # DUMMY.
    set_world_inactive()
    # TODO: remove things, map
    worlddb_value_setter("MAP_LENGTH", 1, 256)(maplength_string)


def command_worldactive(worldactive_string):
    # DUMMY.
    val = integer_test(worldactive_string, 0, 1)
    if val:
        if 0 != world_db["WORLD_ACTIVE"] and 0 == val:
            set_world_inactive()
        elif 0 == world_db["WORLD_ACTIVE"]:
            wait_exists = False
            player_exists = False
            map_exists = False
            # TODO: perform tests:
            # Is there thing action of name 'wait'?
            # Is there a player thing?
            # Is there a map?
            if wait_exists and player_exists and map_exists:
                # TODO: rebuild al things' FOVs, map memories
                world_db["WORLD_ACTIVE"] = 1


def command_taid(id_string):
    """Set ID of ThingAction to manipulate. ID unused? Create new ThingAction.

    The ID of the ThingAction to manipulate is stored as command_taid.id. If
    the integer of the input value is valid (>= 0 and <= 255), but 0, a new ID
    is calculated: The lowest unused ID >0 and <= 255. A new ThingAction's
    "TA_EFFORT" defaults to 1, its "TA_NAME" to "wait".
    """
    id = integer_test(id_string, 0, 255)
    if id:
        if id in world_db["ThingActions"]:
            command_taid.id = id
        else:
            if 0 == id:
                while 1:
                    id = id + 1
                    if id not in world_db["ThingActions"]:
                        break
                if id > 255:
                    print("Ignoring: "
                          "No unused ID available to add to ID list.")
                    return
            world_db["ThingActions"][id] = {
                "TA_EFFORT": 1,
                "TA_NAME": "wait"
            }
            command_taid.id = id


def command_taeffort(str_int):
    """Set to int(str_int) effort value of ThingAction of command_taid.id."""
    if hasattr(command_taid, "id"):
        val = integer_test(str_int, 0, 255)
        if val:
            world_db["ThingActions"][command_taid.id]["TA_EFFORT"] = val
    else:
        print("No thing action defined to manipulate yet.")



"""Commands database.

Map command start tokens to ([0]) number of expected command arguments, ([1])
the command's meta-ness (i.e. is it to be written to the record file, is it to
be ignored in replay mode if read from server input file), and ([2]) a function
to be called on it.
"""
commands_db = {
    "QUIT": (0, True, command_quit),
    "PING": (0, True, command_ping),
    "MAKE_WORLD": (1, False, command_makeworld),
    "SEED_MAP": (1, False, command_seedmap),
    "SEED_RANDOMNESS": (1, False, worlddb_value_setter("SEED_RANDOMNESS", 0,
                                                       4294967295)),
    "TURN": (1, False, worlddb_value_setter("TURN", 0, 65535)),
    "PLAYER_TYPE": (1, False, worlddb_value_setter("PLAYER_TYPE", 0, 255)),
    "MAP_LENGTH": (1, False, command_maplength),
    "WORLD_ACTIVE": (1, False, command_worldactive),
    "TA_ID": (1, False, command_taid),
    "TA_EFFORT": (1, False, command_taeffort)
}


"""World state database. With sane default values."""
world_db = {
    "TURN": 1,
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
    "kicked_by_rival": False
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
