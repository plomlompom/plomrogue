# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


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
