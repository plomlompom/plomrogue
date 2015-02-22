import argparse
import errno
import os
import shlex
import shutil
import time


def setup_server_io(io_db):
    """Fill IO files DB with proper file( path)s. Write process IO test string.

    Decide file paths. Ensure IO files directory at server/. Remove any old in
    file if found. Set up new in file (io_db["file_in"]) for reading at
    io_db["path_in"], and new out file (io_db["file_out"]) for writing at
    io_db["path_out"]. Start out file with process hash line of format PID +
    " " + floated UNIX time (io_db["teststring"]). Run detect_atomic_leftover
    on io_db["path_record"] and io_db["path_save"].
    """
    io_dir = "server/"
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


def cleanup_server_io(io_db):
    """Close and (if io_db["kicked_by_rival"] false) remove files in io_db."""
    def helper(file_key, path_key):
        if file_key in io_db:
            io_db[file_key].close()
            if not io_db["kicked_by_rival"]:
                os.remove(io_db[path_key])
    helper("file_out", "path_out")
    helper("file_in", "path_in")
    helper("file_worldstate", "path_worldstate")


def detect_atomic_leftover(path, tmp_suffix):
    """Raise explained SystemExit if file is found at path + tmp_suffix."""
    path_tmp = path + tmp_suffix
    msg = "Found file '" + path_tmp + "' that may be a leftover from an " \
          "aborted previous attempt to write '" + path + "'. Aborting until " \
          "the matter is resolved by removing it from its current path."
    if os.access(path_tmp, os.F_OK):
        raise SystemExit(msg)


def obey(cmd, io_db, prefix):
    """"""
    print("input " + prefix + ": " + cmd)
    try:
        tokens = shlex.split(cmd, comments=True)
    except ValueError as err:
        print("Can't tokenize command string: " + str(err) + ".")
        return
    if 0 == len(tokens):
        pass
    elif "PING" == tokens[0] and 1 == len(tokens):
        io_db["file_out"].write("PONG\n")
    elif "QUIT" == tokens[0] and 1 == len(tokens):
        record("# " + cmd, path_recordfile)
        raise SystemExit("received QUIT command")
    elif "MAKE_WORLD" == tokens[0] and 2 == len(tokens):
        print("I would generate a new world now, if only I knew how.")
        record(cmd, io_db)
    else:
        print("Invalid command/argument, or bad number of tokens.")


def record(cmd, io_db):
    """Append cmd string plus newline to file at path_recordfile. (Atomic.)"""
    # This misses some optimizations from the original record(), namely only
    # finishing the atomic write with expensive flush() and fsync() every 15
    # seconds unless explicitely forced. Implement as needed.
    path_tmp = io_db["path_record"] + io_db["tmp_suffix"]
    if os.access(io_db["path_record"], os.F_OK):
        shutil.copyfile(io_db["path_record"], path_tmp)
    file = open(path_tmp, "a")
    file.write(cmd + "\n")
    file.flush()
    os.fsync(file.fileno())
    file.close()
    if os.access(io_db["path_record"], os.F_OK):
        os.remove(io_db["path_record"])
    os.rename(path_tmp, io_db["path_record"])


def obey_lines_in_file(path, name, break_test=None):
    """Call obey() on each line of path's file, use name in input prefix.

    If break_test function is set, only read the file until it returns True.
    """
    file = open(path, "r")
    line_n = 1
    for line in file.readlines():
        if None != break_test and break_test():
            break
        obey(line.rstrip(), io_db, name + "file line " + str(line_n))
        line_n = line_n + 1
    file.close()


def make_turn_tester(turn_to_compare, world_db):
    """Return tester whether world_db["turn"] greater/equal turn_to_compare."""
    def turn_tester():
        return world_db["turn"] >= turn_to_compare
    return turn_tester


def parse_command_line_arguments():
    """Return settings values read from command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', nargs='?', type=int, dest='replay', const=1,
                        action='store')
    opts, unknown = parser.parse_known_args()
    return opts


def server_test(io_db):
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


io_db = {}
world_db = {}
try:
    opts = parse_command_line_arguments()
    setup_server_io(io_db)
    # print("DUMMY: Run game.")
    if None != opts.replay:
        if opts.replay < 1:
            opts.replay = 1
        print("Replay mode. Auto-replaying up to turn " + str(opts.replay) +
              " (if so late a turn is to be found).")
        if not os.access(io_db["path_record"], os.F_OK):
            raise SystemExit("No record file found to replay.")
        world_db["turn"] = 0
        break_tester = make_turn_tester(opts.replay, world_db)
        obey_lines_in_file(io_db["path_record"], "record ", break_tester)
        # what to do next?
    else:
        if os.access(io_db["path_save"], os.F_OK):
            obey_lines_in_file(io_db["path_save"], "save")
        else:
            if not os.access(io_db["path_worldconf"], os.F_OK):
                msg = "No world config file from which to start a new world."
                raise SystemExit(msg)
            obey_lines_in_file(io_db["path_worldconf"], "world config ")
            obey("MAKE_WORLD " + str(int(time.time())), io_db, "in file")
        while 1:
            server_test(io_db)
        # print("DUMMY: Run io_loop().")
except SystemExit as exit:
    print("ABORTING: " + exit.args[0])
except:
    print("SOMETHING WENT WRONG IN UNEXPECTED WAYS")
    raise
finally:
    cleanup_server_io(io_db)
    # print("DUMMY: (Clean up C heap.)")
