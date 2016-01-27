io = {
    "path_out": "server_run/in",
    "path_in": "server_run/out",
    "path_worldstate": "server_run/worldstate",
    "worldstate_read_order": [
        ["lifepoints", "int"],
        ["satiation", "int"],
        ["inventory", "lines"],
        ["avatar_position", "int", 0],
        ["avatar_position", "int", 1],
        ["map_size", "int"],
        ["fov_map", "map"],
        ["mem_map", "map"]
    ]
}
