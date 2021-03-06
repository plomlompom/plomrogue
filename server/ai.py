# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import world_db


def get_dir_to_target(t, filter):
    """Try to set T_COMMAND/T_ARGUMENT for move to "filter"-determined target.

    The path-wise nearest target is chosen, via the shortest available path.
    Target must not be t. On succcess, return positive value, else False.
    Filters:
    "a": Thing in FOV is animate, but of ThingType, starts out weaker than t
         is, and its corpse would be healthy food for t
    "f": move away from an enemy – any visible actor whose thing type has more
         TT_LIFEPOINTS than t LIFEPOINTS, and might find t's corpse healthy
         food – if it is closer than n steps, where n will shrink as t's hunger
         grows; if enemy is too close, move towards (attack) the enemy instead;
         if no fleeing is possible, nor attacking useful, wait; don't tread on
         non-enemies for fleeing
    "c": Thing in memorized map is consumable of sufficient nutrition for t
    "s": memory map cell with greatest-reachable degree of unexploredness
    """
    from server.utils import rand, libpr, c_pointer_to_bytearray, \
            c_pointer_to_string
    from server.config.world_data import symbols_passable
    tt = world_db["ThingTypes"][t["T_TYPE"]]

    def zero_score_map_where_char_on_memdepthmap(c):
        # OUTSOURCED FOR PERFORMANCE REASONS TO libplomrogue.so:
        # for i in [i for i in range(world_db["MAP_LENGTH"] ** 2)
        #           if t["T_MEMDEPTHMAP"][i] == mem_depth_c[0]]:
        #     set_map_score(i, 0)
        map = c_pointer_to_bytearray(t["T_MEMDEPTHMAP"])
        if libpr.zero_score_map_where_char_on_memdepthmap(c, map):
            raise RuntimeError("No score map allocated for "
                               "zero_score_map_where_char_on_memdepthmap().")

    def set_map_score(pos, score):
        test = libpr.set_map_score(pos, score)
        if test:
            raise RuntimeError("No score map allocated for set_map_score().")

    def get_map_score(pos):
        result = libpr.get_map_score(pos)
        if result < 0:
            raise RuntimeError("No score map allocated for get_map_score().")
        return result

    def animates_in_fov(maplength):
        return [Thing for Thing in world_db["Things"].values()
                if Thing["T_LIFEPOINTS"] and not Thing["carried"]
                and 118 == t["fovmap"][Thing["pos"]] and not Thing == t]

    def good_attack_target(v):
        eat_cost = tt["eat_vs_hunger_threshold"]
        ty = world_db["ThingTypes"][v["T_TYPE"]]
        type_corpse = world_db["ThingTypes"][ty["TT_CORPSE_ID"]]
        if t["T_LIFEPOINTS"] > ty["TT_LIFEPOINTS"] \
        and type_corpse["TT_TOOL"] == "food" \
        and type_corpse["TT_TOOLPOWER"] > eat_cost:
            return True
        return False

    def good_flee_target(m):
        corpse_type = world_db["ThingTypes"][tt["TT_CORPSE_ID"]]
        targetness = 0 if corpse_type["TT_TOOL"] != "food" \
                       else corpse_type["TT_TOOLPOWER"]
        type = world_db["ThingTypes"][m["T_TYPE"]]
        if t["T_LIFEPOINTS"] < type["TT_LIFEPOINTS"] \
            and targetness > type["eat_vs_hunger_threshold"]:
            return True
        return False

    def seeing_thing():
        def exists(gen):
            try:
                next(gen)
            except StopIteration:
                return False
            return True
        maplength = world_db["MAP_LENGTH"]
        if t["fovmap"] and "a" == filter:
            return exists(Thing for Thing in animates_in_fov(maplength)
                                if good_attack_target(Thing))
        elif t["fovmap"] and "f" == filter:
            return exists(Thing for Thing in animates_in_fov(maplength)
                                if good_flee_target(Thing))
        elif t["T_MEMMAP"] and "c" == filter:
            eat_cost = tt["eat_vs_hunger_threshold"]
            ord_blank = ord(" ")
            map_len = world_db["MAP_LENGTH"]
            return exists(mt for mt in t["T_MEMTHING"]
                          if ord_blank != t["T_MEMMAP"][mt[1] * map_len + mt[2]]
                          and world_db["ThingTypes"][mt[0]]["TT_TOOL"] == "food"
                          and world_db["ThingTypes"][mt[0]]["TT_TOOLPOWER"]
                             > eat_cost)
        return False

    def set_cells_passable_on_memmap_to_65534_on_scoremap():
        # OUTSOURCED FOR PERFORMANCE REASONS TO libplomrogue.so:
        # memmap = t["T_MEMMAP"]
        # for i in [i for i in range(world_db["MAP_LENGTH"] ** 2)
        #            if memmap[i] in symbols_passable]:
        #     set_map_score(i, 65534) # i.e. 65535-1
        scoremap = c_pointer_to_bytearray(t["T_MEMMAP"])
        passable_string = c_pointer_to_string(symbols_passable)
        if libpr.set_cells_passable_on_memmap_to_65534_on_scoremap(scoremap,
                    passable_string):
            raise RuntimeError("No score map allocated for set_cells_passable"
                               "_on_memmap_to_65534_on_scoremap().")

    def init_score_map():
        test = libpr.init_score_map()
        if test:
            raise RuntimeError("Malloc error in init_score_map().")
        ord_v = ord("v")
        ord_blank = ord(" ")
        set_cells_passable_on_memmap_to_65534_on_scoremap()
        maplen = world_db["MAP_LENGTH"]
        if "a" == filter:
            [set_map_score(Thing["pos"], 0) for
             Thing in animates_in_fov(maplen) if good_attack_target(Thing)]
        elif "f" == filter:
            [set_map_score(Thing["pos"], 0) for
             Thing in animates_in_fov(maplen) if good_flee_target(Thing)]
        elif "c" == filter:
            eat_cost = tt["eat_vs_hunger_threshold"]
            ord_blank = ord(" ")
            [set_map_score(mt[1] * maplen + mt[2], 0)
             for mt in t["T_MEMTHING"]
             if ord_blank != t["T_MEMMAP"][mt[1] * maplen + mt[2]]
             if world_db["ThingTypes"][mt[0]]["TT_TOOL"] == "food"
             if world_db["ThingTypes"][mt[0]]["TT_TOOLPOWER"] > eat_cost]
        elif "s" == filter:
            zero_score_map_where_char_on_memdepthmap(mem_depth_c[0])
        if "f" == filter:
            [set_map_score(Thing["pos"], 65535)
             for Thing in animates_in_fov(maplen)
             if get_map_score(Thing["pos"])]
        elif "a" != filter:
            [set_map_score(Thing["pos"], 65535)
             for Thing in animates_in_fov(maplen)]

    def rand_target_dir(neighbors, cmp, dirs):
        candidates = []
        n_candidates = 0
        for i in range(len(dirs)):
            if cmp == neighbors[i]:
                candidates.append(dirs[i])
                n_candidates += 1
        return candidates[rand.next() % n_candidates] if n_candidates else 0

    def get_neighbor_scores(dirs, eye_pos):
        scores = []
        if libpr.ready_neighbor_scores(eye_pos):
            raise RuntimeError("No score map allocated for " +
                               "ready_neighbor_scores.()")
        for i in range(len(dirs)):
            scores.append(libpr.get_neighbor_score(i))
        return scores

    def get_dir_from_neighbors():
        import math
        dir_to_target = False
        dirs = "edcxsw"
        eye_pos = t["pos"]
        neighbors = get_neighbor_scores(dirs, eye_pos)
        minmax_start = 0 if "f" == filter else 65535 - 1
        minmax_neighbor = minmax_start
        for i in range(len(dirs)):
            if ("f" == filter and get_map_score(eye_pos) < neighbors[i] and
                minmax_neighbor < neighbors[i] and 65535 != neighbors[i]) \
               or ("f" != filter and minmax_neighbor > neighbors[i]):
                minmax_neighbor = neighbors[i]
        if minmax_neighbor != minmax_start:
            dir_to_target = rand_target_dir(neighbors, minmax_neighbor, dirs)
        if "f" == filter:
            distance = get_map_score(eye_pos)
            fear_distance = world_db["MAP_LENGTH"]
            if t["T_SATIATION"] < 0 and math.sqrt(-t["T_SATIATION"]) > 0:
                fear_distance = fear_distance / math.sqrt(-t["T_SATIATION"])
            attack_distance = 1
            if not dir_to_target:
                if attack_distance >= distance:
                    dir_to_target = rand_target_dir(neighbors,
                                                    distance - 1, dirs)
                elif fear_distance >= distance:
                    t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                                      if
                                      world_db["ThingActions"][id]["TA_NAME"]
                                      == "wait"][0]
                    return 1
            elif dir_to_target and fear_distance < distance:
                dir_to_target = 0
        return dir_to_target

    dir_to_target = False
    mem_depth_c = b' '
    run_i = 9 + 1 if "s" == filter else 1
    while run_i and not dir_to_target and ("s" == filter or seeing_thing()):
        run_i -= 1
        init_score_map()
        mem_depth_c = b'9' if b' ' == mem_depth_c \
            else bytes([mem_depth_c[0] - 1])
        if libpr.dijkstra_map():
            raise RuntimeError("No score map allocated for dijkstra_map().")
        dir_to_target = get_dir_from_neighbors()
        libpr.free_score_map()
        if dir_to_target and str == type(dir_to_target):
            t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                              if world_db["ThingActions"][id]["TA_NAME"]
                              == "move"][0]
            t["T_ARGUMENT"] = ord(dir_to_target)
    return dir_to_target


def standing_on_food(t):
    """Return True/False whether t is standing on healthy consumable."""
    tt = world_db["ThingTypes"][t["T_TYPE"]]
    eat_cost = tt["eat_vs_hunger_threshold"]
    for id in [id for id in world_db["Things"] if world_db["Things"][id] != t
               if not world_db["Things"][id]["carried"]
               if world_db["Things"][id]["pos"] == t["pos"]
               if world_db["ThingTypes"][world_db["Things"][id]["T_TYPE"]]
                  ["TT_TOOL"] == "food"
               if world_db["ThingTypes"][world_db["Things"][id]["T_TYPE"]]
                  ["TT_TOOLPOWER"] > eat_cost]:
        return True
    return False


def get_inventory_slot_to_consume(t):
    """Return invent. slot of healthiest consumable(if any healthy),else -1."""
    cmp_food = -1
    selection = -1
    i = 0
    tt = world_db["ThingTypes"][t["T_TYPE"]]
    eat_cost = tt["eat_vs_hunger_threshold"]
    for id in t["T_CARRIES"]:
        type = world_db["Things"][id]["T_TYPE"]
        if world_db["ThingTypes"][type]["TT_TOOL"] == "food" \
           and world_db["ThingTypes"][type]["TT_TOOLPOWER"]:
            nutvalue = world_db["ThingTypes"][type]["TT_TOOLPOWER"]
            tmp_cmp = abs(t["T_SATIATION"] + nutvalue - eat_cost)
            if (cmp_food < 0 and tmp_cmp < abs(t["T_SATIATION"])) \
            or tmp_cmp < cmp_food:
                cmp_food = tmp_cmp
                selection = i
        i += 1
    return selection


def ai(t):
    """Determine next command/argment for actor t via AI algorithms."""
    t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                      if world_db["ThingActions"][id]["TA_NAME"] == "wait"][0]
    eating = len([id for id in world_db["ThingActions"]
                  if world_db["ThingActions"][id]["TA_NAME"] == "use"]) > 0
    picking = len([id for id in world_db["ThingActions"]
                   if world_db["ThingActions"][id]["TA_NAME"] == "pickup"]) > 0
    if eating and picking:
        if get_dir_to_target(t, "f"):
            return
        sel = get_inventory_slot_to_consume(t)
        from server.config.ai import ai_hook_pickup_test
        if -1 != sel:
            t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                              if world_db["ThingActions"][id]["TA_NAME"]
                                 == "use"][0]
            t["T_ARGUMENT"] = sel
        elif standing_on_food(t) and ai_hook_pickup_test(t):
                t["T_COMMAND"] = [id for id in world_db["ThingActions"]
                                  if world_db["ThingActions"][id]["TA_NAME"]
                                  == "pickup"][0]
        else:
            going_to_known_food_spot = get_dir_to_target(t, "c")
            if not going_to_known_food_spot:
                aiming_for_walking_food = get_dir_to_target(t, "a")
                if not aiming_for_walking_food:
                    get_dir_to_target(t, "s")
