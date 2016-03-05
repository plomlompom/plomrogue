import ctypes


def mv_yx_in_dir_legal(dir, y, x):
    """Wrapper around libpr.mv_yx_in_dir_legal to simplify its use."""
    dir_c = dir.encode("ascii")[0]
    test = libpr.mv_yx_in_dir_legal_wrap(dir_c, y, x)
    if -1 == test:
        raise RuntimeError("Too much wrapping in mv_yx_in_dir_legal_wrap()!")
    return (test, libpr.result_y(), libpr.result_x())


class RandomnessIO:
    """"Interface to libplomrogue's pseudo-randomness generator."""

    def set_seed(self, seed):
        libpr.seed_rrand(1, seed)

    def get_seed(self):
        return libpr.seed_rrand(0, 0)

    def next(self):
        return libpr.rrand()

    seed = property(get_seed, set_seed)


def integer_test(val_string, min, max=None):
    """Return val_string if integer >= min & (if max set) <= max, else None."""
    try:
        val = int(val_string)
        if val < min or (max is not None and val > max):
            raise ValueError
        return val
    except ValueError:
        msg = "Ignoring: Please use integer >= " + str(min)
        if max is not None:
            msg += " and <= " + str(max)
        msg += "."
        print(msg)
        return None


def id_setter(id, category, id_store=False, start_at_1=False):
    """Set ID of object of category to manipulate. ID unused? Create new one.

    The ID is stored as id_store.id (if id_store is set). If the integer of the
    input is valid (if start_at_1, >= 0, else >= -1), but <0 or (if start_at_1)
    <1, calculate new ID: lowest unused ID >=0 or (if start_at_1) >= 1. None is
    always returned when no new object is created, else the new object's ID.
    """
    from server.config.world_data import world_db
    min = 0 if start_at_1 else -1
    if str == type(id):
        id = integer_test(id, min)
    if None != id:
        if id in world_db[category]:
            if id_store:
                id_store.id = id
            return None
        else:
            if (start_at_1 and 0 == id) \
               or ((not start_at_1) and (id < 0)):
                id = 0 if start_at_1 else -1
                while 1:
                    id = id + 1
                    if id not in world_db[category]:
                        break
            if id_store:
                id_store.id = id
    return id


def prep_library():
    """Prepare ctypes library at ./libplomrogue.so"""
    import os
    libpath = ("./libplomrogue.so")
    if not os.access(libpath, os.F_OK):
        raise SystemExit("No library " + libpath + ", run ./build.sh first?")
    libpr = ctypes.cdll.LoadLibrary(libpath)
    libpr.seed_rrand.restype = ctypes.c_uint32
    return libpr


def c_pointer_to_bytearray(ba):
    """Return C char * pointer to ba."""
    ty = ctypes.c_char * len(ba)
    return ty.from_buffer(ba)


def c_pointer_to_string(string):
    """Return C char * pointer to string."""
    p = ctypes.c_char_p(string.encode("ascii"))
    return p


def parse_command_line_arguments():
    """Return settings values read from command line arguments."""
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', nargs='?', type=int, dest='replay', const=1,
                        action='store')
    parser.add_argument('-l', nargs="?", const="save", dest='savefile',
                        action="store")
    parser.add_argument('-w', type=str, default="confserver/world",
                        dest='worldconf', action='store')
    parser.add_argument('-v', dest='verbose', action='store_true')
    opts, unknown = parser.parse_known_args()
    return opts


libpr = prep_library()
rand = RandomnessIO()
opts = parse_command_line_arguments()
