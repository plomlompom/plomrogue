# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def update_map_memory(t, age_map=True):
    """Update t's T_MEMMAP with what's in its FOV now,age its T_MEMMEPTHMAP."""
    from server.utils import c_pointer_to_bytearray, libpr
    from server.config.world_data import world_db

    def age_some_memdepthmap_on_nonfov_cells():
        # OUTSOURCED FOR PERFORMANCE REASONS TO libplomrogue.so:
        # ord_v = ord("v")
        # ord_0 = ord("0")
        # ord_9 = ord("9")
        # for pos in [pos for pos in range(world_db["MAP_LENGTH"] ** 2)
        #             if not ord_v == t["fovmap"][pos]
        #             if ord_0 <= t["T_MEMDEPTHMAP"][pos]
        #             if ord_9 > t["T_MEMDEPTHMAP"][pos]
        #             if not rand.next() % (2 **
        #                                   (t["T_MEMDEPTHMAP"][pos] - 48))]:
        #     t["T_MEMDEPTHMAP"][pos] += 1
        memdepthmap = c_pointer_to_bytearray(t["T_MEMDEPTHMAP"])
        fovmap = c_pointer_to_bytearray(t["fovmap"])
        libpr.age_some_memdepthmap_on_nonfov_cells(memdepthmap, fovmap)

    def update_mem_and_memdepthmap_via_fovmap():
        # OUTSOURCED FOR PERFORMANCE REASONS TO libplomrogue.so:
        # for pos in [pos for pos in range(world_db["MAP_LENGTH"] ** 2)
        #             if ord_v == t["fovmap"][pos]]:
        #     t["T_MEMDEPTHMAP"][pos] = ord_0
        #     t["T_MEMMAP"][pos] = world_db["MAP"][pos]
        memdepthmap = c_pointer_to_bytearray(t["T_MEMDEPTHMAP"])
        memmap = c_pointer_to_bytearray(t["T_MEMMAP"])
        fovmap = c_pointer_to_bytearray(t["fovmap"])
        map = c_pointer_to_bytearray(world_db["MAP"])
        libpr.update_mem_and_memdepthmap_via_fovmap(map, fovmap, memdepthmap,
                                                    memmap)

    if not t["T_MEMMAP"]:
        t["T_MEMMAP"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
    if not t["T_MEMDEPTHMAP"]:
        t["T_MEMDEPTHMAP"] = bytearray(b' ' * (world_db["MAP_LENGTH"] ** 2))
    update_mem_and_memdepthmap_via_fovmap()
    if age_map:
        age_some_memdepthmap_on_nonfov_cells()
    ord_v = ord("v")
    t["T_MEMTHING"] = [mt for mt in t["T_MEMTHING"]
                       if ord_v != t["fovmap"][(mt[1] * world_db["MAP_LENGTH"])
                                               + mt[2]]]
    for id in [id for id in world_db["Things"]
               if not world_db["Things"][id]["carried"]]:
        type = world_db["Things"][id]["T_TYPE"]
        if not world_db["ThingTypes"][type]["TT_LIFEPOINTS"]:
            y = world_db["Things"][id]["T_POSY"]
            x = world_db["Things"][id]["T_POSX"]
            if ord_v == t["fovmap"][(y * world_db["MAP_LENGTH"]) + x]:
                t["T_MEMTHING"].append((type, y, x))
