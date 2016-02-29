# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.commands import command_plugin, command_quit, command_ping, \
    command_thingshere, command_makeworld, command_seedrandomness, setter, \
    command_maplength, command_worldactive, setter_map, command_taid, \
    command_taname, command_ttid, command_ttname, command_tttool, \
    command_ttsymbol, command_ttcorpseid, command_tid, command_tcommand, \
    command_ttype, command_tcarries, command_tmemthing, setter_tpos, \
    play_wait, play_move, play_pickup, play_drop, play_use, command_ai


"""Commands database.

Map command start tokens to ([0]) number of expected command arguments, ([1])
the command's meta-ness (i.e. is it to be written to the record file, is it to
be ignored in replay mode if read from server input file), and ([2]) a function
to be called on it.
"""
commands_db = {
    "PLUGIN": (1, False, command_plugin),
    "QUIT": (0, True, command_quit),
    "PING": (0, True, command_ping),
    "THINGS_HERE": (2, True, command_thingshere),
    "MAKE_WORLD": (1, False, command_makeworld),
    "SEED_RANDOMNESS": (1, False, command_seedrandomness),
    "TURN": (1, False, setter(None, "TURN", 0, 65535)),
    "PLAYER_TYPE": (1, False, setter(None, "PLAYER_TYPE", 0)),
    "MAP_LENGTH": (1, False, command_maplength),
    "WORLD_ACTIVE": (1, False, command_worldactive),
    "MAP": (2, False, setter_map("MAP")),
    "TA_ID": (1, False, command_taid),
    "TA_EFFORT": (1, False, setter("ThingAction", "TA_EFFORT", 0, 255)),
    "TA_NAME": (1, False, command_taname),
    "TT_ID": (1, False, command_ttid),
    "TT_NAME": (1, False, command_ttname),
    "TT_TOOL": (1, False, command_tttool),
    "TT_SYMBOL": (1, False, command_ttsymbol),
    "TT_CORPSE_ID": (1, False, command_ttcorpseid),
    "TT_TOOLPOWER": (1, False, setter("ThingType", "TT_TOOLPOWER", 0, 65535)),
    "TT_START_NUMBER": (1, False, setter("ThingType", "TT_START_NUMBER",
                                         0, 255)),
    "TT_PROLIFERATE": (1, False, setter("ThingType", "TT_PROLIFERATE",
                                        0, 65535)),
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
    "wait": (0, False, play_wait),
    "move": (1, False, play_move),
    "pickup": (0, False, play_pickup),
    "drop": (1, False, play_drop),
    "use": (1, False, play_use),
    "ai": (0, False, command_ai)
}
# TODO: Unhandled cases: (Un-)killing animates (esp. player!) with T_LIFEPOINTS.

command_worldactive_test_hook = lambda: True
play_move_attempt_hook = lambda x, y, z: False
play_use_attempt_hook = lambda x, y: None
play_pickup_attempt_hook = lambda x: True
