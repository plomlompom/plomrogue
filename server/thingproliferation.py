# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def thingproliferation(t, prol_map):
    """To chance of 1/TT_PROLIFERATE, create t offspring in open neighbor cell.

    Naturally only works with TT_PROLIFERATE > 0. The neighbor cell must be be
    marked passable in prol_map. If there are several map cell candidates, one
    is selected randomly.
    """
    from server.config.world_data import directions_db, world_db
    from server.config.thingproliferation import field_spreadable, \
        thingprol_plugin_conditions, thingprol_plugin_post_create_hook
    from server.utils import mv_yx_in_dir_legal, rand
    from server.new_thing import new_Thing
    prolscore = world_db["ThingTypes"][t["T_TYPE"]]["TT_PROLIFERATE"]
    if prolscore and (1 == prolscore or 1 == (rand.next() % prolscore)) and \
        thingprol_plugin_conditions(t):
        candidates = []
        for dir in [directions_db[key] for key in sorted(directions_db.keys())]:
            mv_result = mv_yx_in_dir_legal(dir, t["T_POSY"], t["T_POSX"])
            c = str(prol_map[mv_result[1] + world_db["MAP_LENGTH"]
                    + mv_result[2]])
            if mv_result[0] and field_spreadable(c, t):
                candidates.append((mv_result[1], mv_result[2]))
        if len(candidates):
            i = rand.next() % len(candidates)
            id = id_setter(-1, "Things")
            newT = new_Thing(t["T_TYPE"], (candidates[i][0], candidates[i][1]))
            world_db["Things"][id] = newT
            thingprol_plugin_post_create_hook(t)
