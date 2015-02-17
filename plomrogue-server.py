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
    """Close and remove all files in IO files DB."""
    io_db["file_out"].close()
    os.remove(io_db["path_out"])
    io_db["file_in"].close()
    os.remove(io_db["path_in"])
    if "file_worldstate" in io_db:                    # This file's only set up
        io_db["file_worldstate"].close()              # properly when the game
    if os.access(io_db["path_worldstate"], os.F_OK):  # world is active, which
        os.remove(io_db["path_worldstate"])           # is not guaranteed.

io_db = {}
try:
    print("DUMMY: Obey command-line arguments.")
    print("DUMMY: Open files.")
    setup_server_io(io_db)
    print("DUMMY: Run game.")
except:
    print("SOMETHING WENT WRONG\n")
    raise
finally:
    cleanup_server_io(io_db)
    print("DUMMY: (Clean up C heap.)")
