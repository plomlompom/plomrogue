# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.

from server.make_map import make_map
from server.thingproliferation import thingproliferation
from server.make_world import make_world
from server.decrement_lifepoints import decrement_lifepoints
from server.calc_effort import calc_effort

decrement_lifepoints_func = decrement_lifepoints
make_map_func = make_map
thingproliferation_func = thingproliferation
make_world_func = make_world
calc_effort_func = calc_effort
