# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from client.windows import win_info, win_log, win_inventory, win_look, win_map


windows_config = [
    {"config": [1, 33],
     "func": win_info,
     "scroll_hints": True,
     "title": "Info"},
    {"config": [-7, 33],
     "func": win_log,
     "scroll_hints": True,
     "title": "Log"},
    {"config": [4, 16],
     "func": win_inventory,
     "scroll_hints": True,
     "title": "Inventory"},
    {"config": [4, 16],
     "func": win_look,
     "scroll_hints": True,
     "title": "Things here"},
    {"config": [0, -34],
     "func": win_map,
     "scroll_hints": True,
      "title": "Map"}
]
