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
    " " + floated UNIX time (io_db["teststring"]).
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
    if os.access(io_db["path_in"], os.F_OK):
        os.remove(io_db["path_in"])
    io_db["file_in"] = open(io_db["path_in"], "w")
    io_db["file_in"].close()
    io_db["file_in"] = open(io_db["path_in"], "r")


def cleanup_server_io(io_db):
    """Close and remove all files open in IO files DB."""
    def helper(file_key, path_key):
        if file_key in io_db:
            io_db[file_key].close()
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
    """Append cmd string plus newline to file at path_recordfile."""
    # Doesn't yet replace old record() fully.
    path_tmp = io_db["path_record"] + io_db["tmp_suffix"]
    if os.access(io_db["path_record"], os.F_OK):
        shutil.copyfile(io_db["path_record"], path_tmp)
    file = open(path_tmp, "a")
    file.write(cmd + "\n")
    file.flush()
    os.fsync(file.fileno())
    file.close()
    os.rename(path_tmp, io_db["path_record"])


def obey_lines_in_file(path, name):
    """Call obey() on each line of path's file, use name in input prefix."""
    file = open(io_db["path_worldconf"], "r")
    line_n = 1
    for line in file.readlines():
        obey(line.rstrip(), io_db, name + "file line " + str(line_n))
        line_n = line_n + 1
    file.close()


io_db = {}
try:
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', nargs='?', type=int, dest='replay', const=1,
                        action='store')
    setup_server_io(io_db)
    # print("DUMMY: Run game.")
    detect_atomic_leftover(io_db["path_save"], io_db["tmp_suffix"])
    detect_atomic_leftover(io_db["path_record"], io_db["tmp_suffix"])
    opts, unknown = parser.parse_known_args()
    if None != opts.replay:
        if opts.replay < 1:
            opts.replay = 1
        print("Replay mode. Auto-replaying up to turn " + str(opts.replay) +
              " (if so late a turn is to be found).")
        if not os.access(io_db["path_record"], os.F_OK):
            raise SystemExit("No record file found to replay.")
    else:
        if os.access(io_db["path_save"], os.F_OK):
            obey_lines_in_file(io_db["path_save"], "save")
        else:
            if not os.access(io_db["path_worldconf"], os.F_OK):
                msg = "No world config file from which to start a new world."
                raise SystemExit(msg)
            obey_lines_in_file(io_db["path_worldconf"], "world config ")
            obey("MAKE_WORLD " + str(int(time.time())), io_db, "in file")
        # print("DUMMY: Run io_loop().")
except SystemExit as exit:
    print("ABORTING: " + exit.args[0])
except:
    print("SOMETHING WENT WRONG IN UNEXPECTED WAYS")
    raise
finally:
    cleanup_server_io(io_db)
    # print("DUMMY: (Clean up C heap.)")
