import argparse
import errno
import os
import shlex
import shutil
import time


def setup_server_io():
    """Fill IO files DB with proper file( path)s. Write process IO test string.

    Set io_db["kicked_by_rival"] to False. Decide file paths. Ensure IO files
    directory at server/. Remove any old in file if found. Set up new in file
    (io_db["file_in"]) for reading at io_db["path_in"], and new out file
    (io_db["file_out"]) for writing at io_db["path_out"]. Start out file with
    process hash line of format PID + " " + floated UNIX time
    (io_db["teststring"]). Run detect_atomic_leftover on io_db["path_record"]
    and io_db["path_save"].
    """
    io_dir = "server/"
    io_db["kicked_by_rival"] = False
    io_db["path_in"] = io_dir + "in"
    io_db["path_out"] = io_dir + "out"
    io_db["path_worldstate"] = io_dir + "worldstate"
    io_db["path_record"] = "record"
    io_db["path_save"] = "save"
    io_db["path_worldconf"] = "confserver/world"
    io_db["tmp_suffix"] = "_tmp"
    io_db["teststring"] = str(os.getpid()) + " " + str(time.time())
    os.makedirs(io_dir, exist_ok=True)
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


def detect_atomic_leftover(path, tmp_suffix):
    """Raise explained SystemExit if file is found at path + tmp_suffix."""
    path_tmp = path + tmp_suffix
    msg = "Found file '" + path_tmp + "' that may be a leftover from an " \
          "aborted previous attempt to write '" + path + "'. Aborting until " \
          "the matter is resolved by removing it from its current path."
    if os.access(path_tmp, os.F_OK):
        raise SystemExit(msg)


def obey(command, prefix, replay=False, do_record=False):
    """Call function from commands_db mapped to command's first token.

    The command string is tokenized by shlex.split(comments=True). If replay is
    set, a non-meta command from the commands_db merely triggers obey() on the
    next command from the records file. Non-meta commands are recorded in
    non-replay mode if do_record is set. The prefix string is inserted into the
    server's input message between its beginning 'input ' and ':'. All activity
    is preceded by a call to server_test().
    """
    server_test()
    print("input " + prefix + ": " + command)
    try:
        tokens = shlex.split(command, comments=True)
    except ValueError as err:
        print("Can't tokenize command string: " + str(err) + ".")
        return
    if len(tokens) > 0 and tokens[0] in commands \
       and len(tokens) >= commands[tokens[0]][0] + 1:
        if commands[tokens[0]][1]:
            commands[tokens[0]][2]()
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
            commands[tokens[0]][2]()
            if do_record:
                record(command)
    else:
        print("Invalid command/argument, or bad number of tokens.")


def record(command):
    """Append command string plus newline to record file. (Atomic.)"""
    # This misses some optimizations from the original record(), namely only
    # finishing the atomic write with expensive flush() and fsync() every 15
    # seconds unless explicitely forced. Implement as needed.
    path_tmp = io_db["path_record"] + io_db["tmp_suffix"]
    if os.access(io_db["path_record"], os.F_OK):
        shutil.copyfile(io_db["path_record"], path_tmp)
    file = open(path_tmp, "a")
    file.write(command + "\n")
    file.flush()
    os.fsync(file.fileno())
    file.close()
    if os.access(io_db["path_record"], os.F_OK):
        os.remove(io_db["path_record"])
    os.rename(path_tmp, io_db["path_record"])


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

    On failure, set io_db["kicked_by_rival"] and raise SystemExit.
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
    while 1:
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


def command_makeworld():
    """Mere dummy so far."""
    print("I would build a whole world now if only I knew how.")


def command_ping():
    """Send PONG line to server output file."""
    io_db["file_out"].write("PONG\n")
    io_db["file_out"].flush()


def command_quit():
    """Abort server process."""
    raise SystemExit("received QUIT command")


"""Commands database.

Map command start tokens to ([0]) minimum number of expected command arguments,
([1]) the command's meta-ness (i.e. is it to be written to the record file, is
it to be ignored in replay mode if read from server input file), and ([2]) a
function to be called on it.
"""
commands_db = {
    "QUIT": (0, True, command_quit),
    "PING": (0, True, command_ping),
    "MAKE_WORLD": (1, False, command_makeworld)
}

io_db = {}
world_db = {}
try:
    opts = parse_command_line_arguments()
    setup_server_io()
    # print("DUMMY: Run game.")
    if None != opts.replay:
        if opts.replay < 1:
            opts.replay = 1
        print("Replay mode. Auto-replaying up to turn " + str(opts.replay) +
              " (if so late a turn is to be found).")
        if not os.access(io_db["path_record"], os.F_OK):
            raise SystemExit("No record file found to replay.")
        world_db["turn"] = 0
        io_db["file_record"] = open(io_db["path_record"], "r")
        io_db["file_record"].prefix = "recod file line "
        io_db["file_record"].line_n = 1
        while world_db["turn"] < opts.replay:
            line = io_db["file_record"].readline()
            if "" == line:
                break
            obey(line.rstrip(), io_db["file_record"].prefix
                 + str(io_db["file_record"].line_n))
            io_db["file_record"].line_n = io_db["file_record"].line_n + 1
        while True:
            obey(read_command(), "in file", replay=True)
    else:
        if os.access(io_db["path_save"], os.F_OK):
            obey_lines_in_file(io_db["path_save"], "save")
        else:
            if not os.access(io_db["path_worldconf"], os.F_OK):
                msg = "No world config file from which to start a new world."
                raise SystemExit(msg)
            obey_lines_in_file(io_db["path_worldconf"], "world config ",
                               do_record=True)
            obey("MAKE_WORLD " + str(int(time.time())), "in file",
                 do_record=True)
        while True:
            obey(read_command(), "in file", do_record=True)
except SystemExit as exit:
    print("ABORTING: " + exit.args[0])
except:
    print("SOMETHING WENT WRONG IN UNEXPECTED WAYS")
    raise
finally:
    cleanup_server_io()
    # print("DUMMY: (Clean up C heap.)")
