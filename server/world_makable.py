# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


def world_makable():
    from server.config.world_data import world_db
    if world_db["MAP_LENGTH"] < 1:
        print("Ignoring: No map length >= 1 defined.")
        return -1 
    player_will_be_generated = False
    playertype = world_db["PLAYER_TYPE"]
    for ThingType in world_db["ThingTypes"]:
        if playertype == ThingType:
            if 0 < world_db["ThingTypes"][ThingType]["TT_START_NUMBER"]:
                player_will_be_generated = True
            break
    if not player_will_be_generated:
        print("Ignoring: No player type with start number >0 defined.")
        return -1 
    wait_action = False
    for ThingAction in world_db["ThingActions"]:
        if "wait" == world_db["ThingActions"][ThingAction]["TA_NAME"]:
            wait_action = True
    if not wait_action:
        print("Ignoring beyond SEED_MAP: " +
              "No thing action with name 'wait' defined.")
        return -1 
    return playertype 
