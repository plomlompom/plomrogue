import argparse
import errno
import os
import time


def setup_server_io(io_db):
    """Fill IO files DB with proper file( path)s. Write process IO test string.

    Ensure IO files directory at server/. Remove any old in file if found. Set
    up new in file (io_db["file_in"]) for reading at io_db["path_in"], and new
    out file (io_db["file_out"]) for writing at io_db["path_out"]. Start out
    file with process hash line of format PID + " " + floated UNIX time
    (io_db["teststring"]). Set worldstate file path io_db["path_worldstate"].
    """
    io_dir = "server/"
    io_db["path_in"] = io_dir + "in"
    io_db["path_out"] = io_dir + "out"
    io_db["path_worldstate"] = io_dir + "worldstate"
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


def detect_atomic_leftover(path):
    """Raise explained HandledException if file is found at path + "_tmp"."""
    path_tmp = path + "_tmp"
    msg = "Found file '" + path_tmp + "' that may be a leftover from an " \
          "aborted previous attempt to write '" + path + "'. Aborting until " \
          "the matter is resolved by removing it from its current path."
    if os.access(path_tmp, os.F_OK):
        raise SystemExit(msg)


def obey(msg):
    """"""
    print("Input: " + msg)


def tokenize(string):
    """Divide string by ' ', \t & " quotes (that also group). Escape with \."""
    charlist_A = list(string)
    i = 0
    for c in charlist_A:
        if "\\" == c and i < len(charlist_A) - 1:
            charlist_A[i] = "remove"
            charlist_A[i + 1] = charlist_A[i + 1] + "_escaped"
        i = i + 1
    charlist_B = []
    for c in charlist_A:
        if "remove" != c:
            charlist_B.append(c)
    in_quotes = 0
    i = 0
    for c in charlist_B:
        if "\"" == c:
            in_quotes = 0 if in_quotes else 1
            if i < len(charlist_B) - 1:
                charlist_B[i] = "separator"
        elif (not in_quotes) and (" " == c or "\t" == c):
            charlist_B[i] = "separator"
        i = i + 1
    list_of_charlists = [[]]
    i = 0
    for c in charlist_B:
        if "separator" == c:
            if [] != list_of_charlists[-1]:
                list_of_charlists.append([])
                i = i + 1
        else:
            list_of_charlists[i].append(c[0])
    tokens = []
    for charlist in list_of_charlists:
        tokens.append("".join(charlist))
    return tokens


io_db = {}
try:
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', nargs='?', type=int, dest='replay', const=1,
                        action='store')
    opts, unknown = parser.parse_known_args()
    setup_server_io(io_db)
    # print("DUMMY: Run game.")
    path_recordfile = "recordfile"
    path_savefile = "savefile"
    detect_atomic_leftover(path_savefile)
    detect_atomic_leftover(path_recordfile)
    if None != opts.replay:
        if opts.replay < 1:
            opts.replay = 1
        print("Replay mode. Auto-replaying up to turn " + str(opts.replay) +
              " (if so late a turn is to be found).")
        if not os.access(path_recordfile, os.F_OK):
            raise SystemExit("No record file found to replay.")
    elif os.access(path_savefile, os.F_OK):
        print(open(path_savefile, "r").read())
    else:
        msg = "MAKE_WORLD " + str(int(time.time()))
        obey(msg)
except SystemExit as exit:
    print("ABORTING: " + exit.args[0])
except:
    print("SOMETHING WENT WRONG IN UNEXPECTED WAYS")
    raise
finally:
    cleanup_server_io(io_db)
    # print("DUMMY: (Clean up C heap.)")
