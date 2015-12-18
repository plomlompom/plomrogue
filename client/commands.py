from client.config.world_data import world_data
from client.io import send
from client.query_mapcell import query_mapcell


def command_quit():
    send("QUIT")
    raise SystemExit("Received QUIT command, forwarded to server, leaving.")


def command_toggle_look_mode():
    if not world_data["look_mode"]:
        world_data["look_mode"] = True
    else:
        world_data["look_mode"] = False
        world_data["map_center"] = world_data["avatar_position"]
        query_mapcell()


def command_sender(string, int_field=None):
    def command_send():
        int_string = ""
        if int_field:
            int_string = " " + str(world_data[int_field])
        send(string + int_string) 
    return command_send


def command_looker(string):
    def command_look():
        if string == "west" \
                and world_data["map_center"][1] > 0:
            world_data["map_center"][1] -= 1
        elif string == "east" \
                and world_data["map_center"][1] < world_data["map_size"] - 1:
            world_data["map_center"][1] += 1
        else:
            y_unevenness = world_data["map_center"][0] % 2
            y_evenness = int(not(y_unevenness))
            if string[6:] == "west" and \
                    world_data["map_center"][1] > -y_unevenness:
                if string[:5] == "north" and world_data["map_center"][0] > 0:
                    world_data["map_center"][0] -= 1
                    world_data["map_center"][1] -= y_evenness
                if string[:5] == "south" and world_data["map_center"][0] \
                        < world_data["map_size"] - 1:
                    world_data["map_center"][0] += 1
                    world_data["map_center"][1] -= y_evenness
            elif string[6:] == "east" and world_data["map_center"][1] \
                    < world_data["map_size"] - y_unevenness:
                if string[:5] == "north" and world_data["map_center"][0] > 0:
                    world_data["map_center"][0] -= 1
                    world_data["map_center"][1] += y_unevenness
                if string[:5] == "south" and world_data["map_center"][0] \
                        < world_data["map_size"] - 1:
                    world_data["map_center"][0] += 1
                    world_data["map_center"][1] += y_unevenness
        query_mapcell()
    return command_look


def command_look_scroller(string):
    def command_look_scroll():
        win_size = next(win["size"] for win in windows
                                    if win["func"] == win_look)
        if string == "up" and world_data["look_scroll"] > 0:
            world_data["look_scroll"] -= 1
        elif string == "down" and world_data["look_scroll"] \
                < len(world_data["look"]) - win_size[0]:
            world_data["look_scroll"] += 1
    return command_look_scroll


def command_inventory_selector(string):
    def command_inventory_select():
        if string == "up" and world_data["inventory_selection"] > 0:
            world_data["inventory_selection"] -= 1
        elif string == "down" and world_data["inventory_selection"] \
                < len(world_data["inventory"]) - 1:
            world_data["inventory_selection"] += 1
    return command_inventory_select
