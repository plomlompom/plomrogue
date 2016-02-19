# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.worldstate_write_helpers import write_inventory, write_fov_map, \
        write_mem_map


"""File IO database."""
io_db = {
    "path_save": "save",
    "path_record": "record_save",
    "path_server": "server_run/",
    "path_in": "server_run/in",
    "path_out": "server_run/out",
    "path_worldstate": "server_run/worldstate",
    "tmp_suffix": "_tmp",
    "kicked_by_rival": False,
    "worldstate_updateable": False,
    "wait_on_read_fail": 0.03333,
    "max_wait_on_read_fail": 5,
    "save_wait": 15,
    "worldstate_write_order": [
        ["TURN", "world_int"],
        ["T_LIFEPOINTS", "player_int"],
        ["T_SATIATION", "player_int"],
        [write_inventory, "func"],
        ["T_POSY", "player_int"],
        ["T_POSX", "player_int"],
        ["MAP_LENGTH", "world_int"],
        [write_fov_map, "func"],
        [write_mem_map, "func"]
    ]
}
