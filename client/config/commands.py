from client.commands import command_sender, command_look_scroller, \
    command_quit, command_looker, command_inventory_selector, \
    command_toggle_look_mode

commands = {
    "A": (command_sender("ai"),),
    "D": (command_sender("drop", "inventory_selection"),),
    "J": (command_look_scroller("down"),),
    "K": (command_look_scroller("up"),),
    "P": (command_sender("pick_up"),),
    "Q": (command_quit,),
    "U": (command_sender("use", "inventory_selection"),),
    "W": (command_sender("wait"),),
    "c": (command_sender("move south-east"), command_looker("south-east")),
    "d": (command_sender("move east"), command_looker("east")),
    "e": (command_sender("move north-east"), command_looker("north-east")),
    "j": (command_inventory_selector("down"),),
    "k": (command_inventory_selector("up"),),
    "l": (command_toggle_look_mode,),
    "s": (command_sender("move west"), command_looker("west")),
    "w": (command_sender("move north-west"), command_looker("north-west")),
    "x": (command_sender("move south-west"), command_looker("south-west")),
}

