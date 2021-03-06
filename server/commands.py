# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db
from server.config.io import io_db
from server.io import log, strong_write 
from server.utils import integer_test, id_setter
from server.world import set_world_inactive, turn_over, eat_vs_hunger_threshold
from server.update_map_memory import update_map_memory
from server.build_fov_map import build_fov_map


def command_plugin(str_plugin):
    """Run code in plugins/[str_plugin]."""
    import os
    if (str_plugin.replace("_", "").isalnum()
        and os.access("plugins/server/" + str_plugin + ".py", os.F_OK)):
        exec(open("plugins/server/" + str_plugin + ".py").read())
        world_db["PLUGIN"] += [str_plugin]
        return
    print("Bad plugin name:", str_plugin)


def command_ping():
    """Send PONG line to server output file."""
    strong_write(io_db["file_out"], "PONG\n")


def command_quit():
    """Abort server process."""
    from server.io import save_world, atomic_write
    from server.utils import opts
    if None == opts.replay:
        if world_db["WORLD_ACTIVE"]:
            save_world()
        atomic_write(io_db["path_record"], io_db["record_chunk"],
            do_append=True)
    raise SystemExit("received QUIT command")


def command_thingshere(str_y, str_x):
    """Write to out file list of Things known to player at coordinate y, x."""
    if world_db["WORLD_ACTIVE"]:
        y = integer_test(str_y, 0, 255)
        x = integer_test(str_x, 0, 255)
        length = world_db["MAP_LENGTH"]
        if None != y and None != x and y < length and x < length:
            pos = (y * world_db["MAP_LENGTH"]) + x
            strong_write(io_db["file_out"], "THINGS_HERE START\n")
            terrain = chr(world_db["Things"][0]["T_MEMMAP"][pos])
            terrain_name = world_db["terrain_names"][terrain]
            strong_write(io_db["file_out"], "terrain: " + terrain_name + "\n")
            if "v" == chr(world_db["Things"][0]["fovmap"][pos]):
                for id in [id for tid in sorted(list(world_db["ThingTypes"]))
                              for id in world_db["Things"]
                              if not world_db["Things"][id]["carried"]
                              if world_db["Things"][id]["T_TYPE"] == tid
                              if pos == world_db["Things"][id]["pos"]]:
                    type = world_db["Things"][id]["T_TYPE"]
                    name = world_db["ThingTypes"][type]["TT_NAME"]
                    strong_write(io_db["file_out"], name + "\n")
            else:
                for mt in [mt for tid in sorted(list(world_db["ThingTypes"]))
                              for mt in world_db["Things"][0]["T_MEMTHING"]
                              if mt[0] == tid if y == mt[1] if x == mt[2]]:
                    name = world_db["ThingTypes"][mt[0]]["TT_NAME"]
                    strong_write(io_db["file_out"], name + "\n")
            strong_write(io_db["file_out"], "THINGS_HERE END\n")
        else:
            print("Ignoring: Invalid map coordinates.")
    else:
        print("Ignoring: Command only works on existing worlds.")


def command_seedrandomness(seed_string):
    """Set rand seed to int(seed_string)."""
    from server.utils import rand
    val = integer_test(seed_string, 0, 4294967295)
    if None != val:
        rand.seed = val


def command_makeworld(seed_string):
    """Call make_world()."""
    val = integer_test(seed_string, 0, 4294967295)
    if None != val:
        from server.make_world import make_world
        make_world(val)


def command_maplength(maplength_string):
    """Redefine map length. Invalidate map, therefore lose all things on it."""
    val = integer_test(maplength_string, 1, 256)
    if None != val:
        from server.utils import libpr
        world_db["MAP_LENGTH"] = val
        world_db["MAP"] = False
        set_world_inactive()
        world_db["Things"] = {}
        libpr.set_maplength(val)


def command_worldactive(worldactive_string):
    """Toggle world_db["WORLD_ACTIVE"] if possible.

    An active world can always be set inactive. An inactive world can only be
    set active with a "wait" ThingAction, and a player Thing (of ID 0), and a
    map. On activation, rebuild all Things' FOVs, and the player's map memory.
    """
    val = integer_test(worldactive_string, 0, 1)
    if None != val:
        if 0 != world_db["WORLD_ACTIVE"]:
            if 0 == val:
                set_world_inactive()
            else:
                print("World already active.")
        elif 0 == world_db["WORLD_ACTIVE"]:
            for ThingAction in world_db["ThingActions"]:
                if "wait" == world_db["ThingActions"][ThingAction]["TA_NAME"]:
                    break
            else:
                print("Ignored: No wait action defined for world to activate.")
                return
            for Thing in world_db["Things"]:
                if 0 == Thing:
                    break
            else:
                print("Ignored: No player defined for world to activate.")
                return
            if not world_db["MAP"]:
                print("Ignoring: No map defined for world to activate.")
                return
            from server.config.commands import command_worldactive_test_hook
            if not command_worldactive_test_hook():
                return
            for tid in world_db["Things"]:
                if world_db["Things"][tid]["T_LIFEPOINTS"]:
                    build_fov_map(world_db["Things"][tid])
                    if 0 == tid:
                        update_map_memory(world_db["Things"][tid], False)
            if not world_db["Things"][0]["T_LIFEPOINTS"]:
                empty_fovmap = bytearray(b" " * world_db["MAP_LENGTH"] ** 2)
                world_db["Things"][0]["fovmap"] = empty_fovmap
            world_db["WORLD_ACTIVE"] = 1


def command_tid(id_string):
    """Set ID of Thing to manipulate. ID unused? Create new one.

    Default new Thing's type to the first available ThingType, others: zero.
    """
    tid = id_setter(id_string, "Things", command_tid)
    if None != tid:
        if world_db["ThingTypes"] == {}:
            print("Ignoring: No ThingType to settle new Thing in.")
            return
        ty = list(world_db["ThingTypes"].keys())[0]
        from server.new_thing import new_Thing
        world_db["Things"][tid] = new_Thing(ty)


def command_ttid(id_string):
    """Set ID of ThingType to manipulate. ID unused? Create new one.

    Set new type's TT_CORPSE_ID to self, other fields to thingtype_defaults.
    """
    ttid = id_setter(id_string, "ThingTypes", command_ttid)
    if None != ttid:
        from server.config.world_data import thingtype_defaults
        world_db["ThingTypes"][ttid] = {}
        for key in thingtype_defaults:
            world_db["ThingTypes"][ttid][key] = thingtype_defaults[key]
        world_db["ThingTypes"][ttid]["TT_CORPSE_ID"] = ttid


def command_taid(id_string):
    """Set ID of ThingAction to manipulate. ID unused? Create new one.

    Default new ThingAction's TA_EFFORT to 1, its TA_NAME to "wait".
    """
    taid = id_setter(id_string, "ThingActions", command_taid, True)
    if None != taid:
        world_db["ThingActions"][taid] = {
            "TA_EFFORT": 1,
            "TA_NAME": "wait"
        }


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


test_Thing_id = test_for_id_maker(command_tid, "Thing")
test_ThingType_id = test_for_id_maker(command_ttid, "ThingType")
test_ThingAction_id = test_for_id_maker(command_taid, "ThingAction")


@test_Thing_id
def command_tcommand(str_int):
    """Set T_COMMAND of selected Thing."""
    val = integer_test(str_int, 0)
    if None != val:
        if 0 == val or val in world_db["ThingActions"]:
            world_db["Things"][command_tid.id]["T_COMMAND"] = val
        else:
            print("Ignoring: ThingAction ID belongs to no known ThingAction.")


@test_Thing_id
def command_ttype(str_int):
    """Set T_TYPE of selected Thing."""
    val = integer_test(str_int, 0)
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
    val = integer_test(str_int, 0)
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
    type = integer_test(str_t, 0)
    posy = integer_test(str_y, 0, 255)
    posx = integer_test(str_x, 0, 255)
    if None != type and None != posy and None != posx:
        if type not in world_db["ThingTypes"] \
           or posy >= world_db["MAP_LENGTH"] or posx >= world_db["MAP_LENGTH"]:
            print("Ignoring: Illegal value for thing type or position.")
        else:
            memthing = (type, posy, posx)
            world_db["Things"][command_tid.id]["T_MEMTHING"].append(memthing)


@test_ThingType_id
def command_ttname(name):
    """Set TT_NAME of selected ThingType."""
    world_db["ThingTypes"][command_ttid.id]["TT_NAME"] = name


@test_ThingType_id
def command_tttool(name):
    """Set TT_TOOL of selected ThingType."""
    world_db["ThingTypes"][command_ttid.id]["TT_TOOL"] = name


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
    val = integer_test(str_int, 0)
    if None != val:
        if val in world_db["ThingTypes"]:
            world_db["ThingTypes"][command_ttid.id]["TT_CORPSE_ID"] = val
        else:
            print("Ignoring: Corpse ID belongs to no known ThignType.")


@test_ThingType_id
def command_ttlifepoints(val_string):
    setter("ThingType", "TT_LIFEPOINTS", 0, 255)(val_string)
    tt = world_db["ThingTypes"][command_ttid.id]
    tt["eat_vs_hunger_threshold"] = eat_vs_hunger_threshold(command_ttid.id)


@test_ThingAction_id
def command_taname(name):
    """Set TA_NAME of selected ThingAction.

    The name must match a valid thing action function. If after the name
    setting no ThingAction with name "wait" remains, call set_world_inactive().
    """
    if name == "wait" or name == "move" or name == "use" or name == "drop" \
       or name == "pickup":
        world_db["ThingActions"][command_taid.id]["TA_NAME"] = name
        if 1 == world_db["WORLD_ACTIVE"]:
            for id in world_db["ThingActions"]:
                if "wait" == world_db["ThingActions"][id]["TA_NAME"]:
                    break
            else:
                set_world_inactive()
    else:
        print("Ignoring: Invalid action name.")


@test_ThingAction_id
def command_taeffort(val_string):
    setter("ThingAction", "TA_EFFORT", 0, 255)(val_string)
    if world_db["ThingActions"][command_taid.id]["TA_NAME"] == "use":
        for ttid in world_db["ThingTypes"]:
            tt = world_db["ThingTypes"][ttid]
            tt["eat_vs_hunger_threshold"] = eat_vs_hunger_threshold(ttid)


def setter(category, key, min, max=None):
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


def setter_map(maptype):
    """Set (world or Thing's) map of maptype's int(str_int)-th line to mapline.

    If no map of maptype exists yet, initialize it with ' ' bytes first.
    """

    def valid_map_line(str_int, mapline):
        val = integer_test(str_int, 0, 255)
        if None != val:
            if val >= world_db["MAP_LENGTH"]:
                print("Illegal value for map line number.")
            elif len(mapline) != world_db["MAP_LENGTH"]:
                print("Map line length is unequal map width.")
            else:
                return val
        return None

    def nonThingMap_helper(str_int, mapline):
        val = valid_map_line(str_int, mapline)
        if None != val:
            length = world_db["MAP_LENGTH"]
            if not world_db["MAP"]:
                map = bytearray(b' ' * (length ** 2))
            else:
                map = world_db["MAP"]
            map[val * length:(val * length) + length] = mapline.encode()
            if not world_db["MAP"]:
                world_db["MAP"] = map

    @test_Thing_id
    def ThingMap_helper(str_int, mapline):
        val = valid_map_line(str_int, mapline)
        if None != val:
            length = world_db["MAP_LENGTH"]
            if not world_db["Things"][command_tid.id][maptype]:
                map = bytearray(b' ' * (length ** 2))
            else:
                map = world_db["Things"][command_tid.id][maptype]
            map[val * length:(val * length) + length] = mapline.encode()
            if not world_db["Things"][command_tid.id][maptype]:
                world_db["Things"][command_tid.id][maptype] = map

    return nonThingMap_helper if maptype == "MAP" else ThingMap_helper



def setter_tpos(axis):
    """Generate setter for T_POSX or  T_POSY of selected Thing.

    If world is active, rebuilds animate things' fovmap, player's memory map.
    """
    @test_Thing_id
    def helper(str_int):
        val = integer_test(str_int, 0, 255)
        if None != val:
            if val < world_db["MAP_LENGTH"]:
                t = world_db["Things"][command_tid.id]
                t["T_POS" + axis] = val
                t["pos"] = t["T_POSY"] * world_db["MAP_LENGTH"] + t["T_POSX"]
                if world_db["WORLD_ACTIVE"] \
                   and world_db["Things"][command_tid.id]["T_LIFEPOINTS"]:
                    build_fov_map(world_db["Things"][command_tid.id])
                    if 0 == command_tid.id:
                        update_map_memory(world_db["Things"][command_tid.id])
            else:
                print("Ignoring: Position is outside of map.")
    return helper


def set_command(action):
    """Set player's T_COMMAND, then call turn_over()."""
    id = [x for x in world_db["ThingActions"]
          if world_db["ThingActions"][x]["TA_NAME"] == action][0]
    world_db["Things"][0]["T_COMMAND"] = id
    turn_over()


def play_wait():
    """Try "wait" as player's T_COMMAND."""
    if world_db["WORLD_ACTIVE"]:
        set_command("wait")


def action_exists(action):
    matching_actions = [x for x in world_db["ThingActions"]
                        if world_db["ThingActions"][x]["TA_NAME"] == action]
    if len(matching_actions) >= 1:
        return True
    print("No appropriate ThingAction defined.")
    return False


def play_pickup():
    """Try "pickup" as player's T_COMMAND"."""
    if action_exists("pickup") and world_db["WORLD_ACTIVE"]:
        t = world_db["Things"][0]
        ids = [tid for tid in world_db["Things"] if tid
               if not world_db["Things"][tid]["carried"]
               if world_db["Things"][tid]["pos"] == t["pos"]]
        from server.config.commands import play_pickup_attempt_hook
        if not len(ids):
             log("NOTHING to pick up.")
        elif play_pickup_attempt_hook(t):
            set_command("pickup")


def play_drop(str_arg):
    """Try "drop" as player's T_COMMAND, int(str_arg) as T_ARGUMENT / slot."""
    if action_exists("drop") and world_db["WORLD_ACTIVE"]:
        t = world_db["Things"][0]
        if 0 == len(t["T_CARRIES"]):
            log("You have NOTHING to drop in your inventory.")
        else:
            val = integer_test(str_arg, 0, 255)
            if None != val and val < len(t["T_CARRIES"]):
                world_db["Things"][0]["T_ARGUMENT"] = val
                set_command("drop")
            else:
                print("Illegal inventory index.")


def play_use(str_arg):
    """Try "use" as player's T_COMMAND, int(str_arg) as T_ARGUMENT / slot."""
    if action_exists("use") and world_db["WORLD_ACTIVE"]:
        t = world_db["Things"][0]
        if 0 == len(t["T_CARRIES"]):
            log("You have NOTHING to use in your inventory.")
        else:
            val = integer_test(str_arg, 0, 255)
            if None != val and val < len(t["T_CARRIES"]):
                tid = t["T_CARRIES"][val]
                tt = world_db["ThingTypes"][world_db["Things"][tid]["T_TYPE"]]
                from server.config.commands import play_use_attempt_hook
                hook_test = play_use_attempt_hook(t, tt)
                if not (tt["TT_TOOL"] == "food" or hook_test):
                    if hook_test != False:
                        log("You CAN'T use this thing.")
                    return
                world_db["Things"][0]["T_ARGUMENT"] = val
                set_command("use")
            else:
                print("Illegal inventory index.")


def play_move(str_arg):
    """Try "move" as player's T_COMMAND, str_arg as T_ARGUMENT / direction."""
    if action_exists("move") and world_db["WORLD_ACTIVE"]:
        from server.config.world_data import directions_db, symbols_passable
        t = world_db["Things"][0]
        if not str_arg in directions_db:
            print("Illegal move direction string.")
            return
        d = ord(directions_db[str_arg])
        from server.utils import mv_yx_in_dir_legal
        move_result = mv_yx_in_dir_legal(chr(d), t["T_POSY"], t["T_POSX"])
        if 1 == move_result[0]:
            pos = (move_result[1] * world_db["MAP_LENGTH"]) + move_result[2]
            if ord("~") == world_db["MAP"][pos]:
                log("You can't SWIM.")
                return
            from server.config.commands import play_move_attempt_hook
            if play_move_attempt_hook(t, d, pos):
                return
            if chr(world_db["MAP"][pos]) in symbols_passable:
                world_db["Things"][0]["T_ARGUMENT"] = d
                set_command("move")
                return
        log("You CAN'T move there.")


def command_ai():
    """Call ai() on player Thing, then turn_over()."""
    from server.ai import ai
    if world_db["WORLD_ACTIVE"]:
        ai(world_db["Things"][0])
        turn_over()
