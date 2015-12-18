from client.windows import win_info, win_log, win_inventory, win_look, win_map

windows_config = [
    {"config": [1, 33], "func": win_info, "title": "Info"},
    {"config": [-7, 33], "func": win_log, "title": "Log"},
    {"config": [4, 16], "func": win_inventory, "title": "Inventory"},
    {"config": [4, 16], "func": win_look, "title": "Things here"},
    {"config": [0, -34], "func": win_map, "title": "Map"}
]
