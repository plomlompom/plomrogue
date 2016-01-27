"""World state database. With sane default values. (Randomness is in rand.)"""
world_db = {
    "TURN": 0,
    "MAP_LENGTH": 64,
    "PLAYER_TYPE": 0,
    "WORLD_ACTIVE": 0,
    "MAP": False,
    "ThingActions": {},
    "ThingTypes": {},
    "Things": {}
}

"""Mapping of direction names to internal direction chars."""
directions_db = {"east": "d", "south-east": "c", "south-west": "x",
                 "west": "s", "north-west": "w", "north-east": "e"}
