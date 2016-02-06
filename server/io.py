# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


import os
import time

from server.config.world_data import world_db
from server.config.io import io_db


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


def safely_remove_worldstate_file():
    from server.io import server_test
    if os.access(io_db["path_worldstate"], os.F_OK):
        server_test()
        os.remove(io_db["path_worldstate"])


def atomic_write(path, text, do_append=False, delete=True):
    """Atomic write of text to file at path, appended if do_append is set."""
    path_tmp = path + io_db["tmp_suffix"]
    mode = "w"
    if do_append:
        mode = "a"
        if os.access(path, os.F_OK):
            from shutil import copyfile 
            copyfile(path, path_tmp)
    file = open(path_tmp, mode)
    strong_write(file, text)
    file.close()
    if delete and os.access(path, os.F_OK):
        os.remove(path)
    os.rename(path_tmp, path)


def strong_write(file, string):
    """Apply write(string), then flush()."""
    file.write(string)
    file.flush()


def setup_server_io():
    """Fill IO files DB with proper file( path)s. Write process IO test string.

    Ensure IO files directory at server/. Remove any old input file if found.
    Set up new input file for reading, and new output file for appending. Start
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
    io_db["save_wait_start"] = 0
    io_db["verbose"] = False
    io_db["record_chunk"] = ""
    os.makedirs(io_db["path_server"], exist_ok=True)
    io_db["file_out"] = open(io_db["path_out"], "a")
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
    helper("file_in", "path_in")
    helper("file_out", "path_out")
    helper("file_worldstate", "path_worldstate")
    if "file_record" in io_db:
        io_db["file_record"].close()

def read_command():
    """Return next newline-delimited command from server in file.

    Keep building return string until a newline is encountered. Pause between
    unsuccessful reads, and after too much waiting, run server_test().
    """
    wait_on_fail = io_db["wait_on_read_fail"]
    max_wait = io_db["max_wait_on_read_fail"] 
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


def log(msg):
    """Send "msg" to log."""
    strong_write(io_db["file_out"], "LOG " + msg + "\n")


def save_world():
    """Save all commands needed to reconstruct current world state."""
    from server.utils import rand

    def quote_escape(string):
        string = string.replace("\u005C", '\u005C\u005C')
        return '"' + string.replace('"', '\u005C"') + '"'

    def mapsetter(key):
        def helper(id=None):
            string = ""
            if key == "MAP" or world_db["Things"][id][key]:
                map = world_db["MAP"] if key == "MAP" \
                                      else world_db["Things"][id][key]
                length = world_db["MAP_LENGTH"]
                for i in range(length):
                    line = map[i * length:(i * length) + length].decode()
                    string = string + key + " " + str(i) + " " + \
                        quote_escape(line) + "\n"
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
        for id in sorted(world_db[category].keys()):
            string = string + id_string + " " + str(id) + "\n"
            for key in sorted(world_db[category][id].keys()):
                if not key in special_keys:
                    x = world_db[category][id][key]
                    argument = quote_escape(x) if str == type(x) else str(x)
                    string = string + key + " " + argument + "\n"
                elif special_keys[key]:
                    string = string + special_keys[key](id)
        return string

    string = ""
    for key in sorted(world_db.keys()):
        if (not isinstance(world_db[key], dict)) and key != "MAP" and \
           key != "WORLD_ACTIVE":
            string = string + key + " " + str(world_db[key]) + "\n"
    string = string + mapsetter("MAP")()
    string = string + helper("ThingActions", "TA_ID")
    string = string + helper("ThingTypes", "TT_ID", {"TT_CORPSE_ID": False})
    for id in sorted(world_db["ThingTypes"].keys()):
        string = string + "TT_ID " + str(id) + "\n" + "TT_CORPSE_ID " + \
            str(world_db["ThingTypes"][id]["TT_CORPSE_ID"]) + "\n"
    string = string + helper("Things", "T_ID",
                             {"T_CARRIES": False, "carried": False,
                              "T_MEMMAP": mapsetter("T_MEMMAP"),
                              "T_MEMTHING": memthing, "fovmap": False,
                              "T_MEMDEPTHMAP": mapsetter("T_MEMDEPTHMAP")})
    for id in sorted(world_db["Things"].keys()):
        if [] != world_db["Things"][id]["T_CARRIES"]:
            string = string + "T_ID " + str(id) + "\n"
            for carried in sorted(world_db["Things"][id]["T_CARRIES"]):
                string = string + "T_CARRIES " + str(carried) + "\n"
    string = string + "SEED_RANDOMNESS " + str(rand.seed) + "\n" + \
        "WORLD_ACTIVE " + str(world_db["WORLD_ACTIVE"])
    atomic_write(io_db["path_save"], string)


def obey(command, prefix, replay=False, do_record=False):
    """Call function from commands_db mapped to command's first token.

    Tokenize command string with shlex.split(comments=True). If replay is set,
    a non-meta command from the commands_db merely triggers obey() on the next
    command from the records file. If not, non-meta commands set
    io_db["worldstate_updateable"] to world_db["WORLD_ACTIVE"], and, if
    do_record is set, are recorded to io_db["record_chunk"], and save_world()
    is called (and io_db["record_chunk"] written) if io_db["save_wait"] seconds
    have passed since the last time it was called. The prefix string is
    inserted into the server's input message between its beginning 'input ' and
    ':'. All activity is preceded by a server_test() call. Commands that start
    with a lowercase letter are ignored when world_db["WORLD_ACTIVE"] is
    False/0.
    """
    import shlex
    from server.config.commands import commands_db
    server_test()
    if io_db["verbose"]:
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
        elif tokens[0][0].islower() and not world_db["WORLD_ACTIVE"]:
            print("Ignoring lowercase-starting commands when world inactive.")
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
                io_db["record_chunk"] += command + "\n"
                if time.time() > io_db["save_wait_start"] + io_db["save_wait"]:
                    atomic_write(io_db["path_record"], io_db["record_chunk"],
                                 do_append=True)
                    if world_db["WORLD_ACTIVE"]:
                        save_world()
                    io_db["record_chunk"] = ""
                    io_db["save_wait"] = time.time()
            io_db["worldstate_updateable"] = world_db["WORLD_ACTIVE"]
    elif 0 != len(tokens):
        print("Invalid command/argument, or bad number of tokens.")


def obey_lines_in_file(path, name, do_record=False):
    """Call obey() on each line of path's file, use name in input prefix."""
    file = open(path, "r")
    line_n = 1
    for line in file.readlines():
        obey(line.rstrip(), name + "file line " + str(line_n),
             do_record=do_record)
        line_n = line_n + 1
    file.close()


def try_worldstate_update():
    """Write worldstate file if io_db["worldstate_updateable"] is set."""
    from server.config.commands import commands_db
    if io_db["worldstate_updateable"]:

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
        ord_v = ord("v")
        for pos in [pos for pos in range(length ** 2)
                        if ord_v == world_db["Things"][0]["fovmap"][pos]]:
            fov[pos] = world_db["MAP"][pos]
        length = world_db["MAP_LENGTH"]
        for id in [id for tid in reversed(sorted(list(world_db["ThingTypes"])))
                      for id in world_db["Things"]
                      if not world_db["Things"][id]["carried"]
                      if world_db["Things"][id]["T_TYPE"] == tid
                      if world_db["Things"][0]["fovmap"][
                           world_db["Things"][id]["T_POSY"] * length
                           + world_db["Things"][id]["T_POSX"]] == ord_v]:
            type = world_db["Things"][id]["T_TYPE"]
            c = ord(world_db["ThingTypes"][type]["TT_SYMBOL"])
            fov[world_db["Things"][id]["T_POSY"] * length
                + world_db["Things"][id]["T_POSX"]] = c
        string = write_map(string, fov)
        mem = world_db["Things"][0]["T_MEMMAP"][:]
        for mt in [mt for tid in reversed(sorted(list(world_db["ThingTypes"])))
                      for mt in world_db["Things"][0]["T_MEMTHING"]
                      if mt[0] == tid]:
             c = world_db["ThingTypes"][mt[0]]["TT_SYMBOL"]
             mem[(mt[1] * length) + mt[2]] = ord(c)
        string = write_map(string, mem)
        atomic_write(io_db["path_worldstate"], string, delete=False)
        strong_write(io_db["file_out"], "WORLD_UPDATED\n")
        io_db["worldstate_updateable"] = False