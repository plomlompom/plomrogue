# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from server.config.world_data import symbols_passable


field_spreadable = lambda x, y: x in symbols_passable
thingprol_plugin_conditions = lambda x: True
thingprol_plugin_post_create_hook = lambda x: None
