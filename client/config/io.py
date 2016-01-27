io = {
    "path_out": "server/in",
    "path_in": "server/out",
    "path_worldstate": "server/worldstate",
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
