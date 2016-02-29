# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def thingproliferation(t, prol_map):
    """To chance of 1/TT_PROLIFERATE, create t offspring in open neighbor cell.

    Naturally only works with TT_PROLIFERATE > 0. The neighbor cell must be be
    marked passable in prol_map. If there are several map cell candidates, one
    is selected randomly.
    """
    from server.config.world_data import directions_db, world_db, \
        thingprol_field_spreadable, thingprol_test_hook, \
        thingprol_post_create_hook
    from server.utils import mv_yx_in_dir_legal, rand, id_setter
    from server.new_thing import new_Thing
    prolscore = world_db["ThingTypes"][t["T_TYPE"]]["TT_PROLIFERATE"]
    if prolscore and (1 == prolscore or 1 == (rand.next() % prolscore)) and \
        thingprol_test_hook(t):
        candidates = []
        for key in sorted(directions_db.keys()):
            mv_result = mv_yx_in_dir_legal(directions_db[key], t["T_POSY"],
                                           t["T_POSX"])
            c = chr(prol_map[mv_result[1] * world_db["MAP_LENGTH"]
                + mv_result[2]])
            if mv_result[0] and thingprol_field_spreadable(c, t):
                from server.io import log
                candidates.append((mv_result[1], mv_result[2]))
        if len(candidates):
            i = rand.next() % len(candidates)
            tid = id_setter(-1, "Things")
            newT = new_Thing(t["T_TYPE"], (candidates[i][0], candidates[i][1]))
            world_db["Things"][tid] = newT
            thingprol_post_create_hook(t)
