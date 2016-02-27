# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.

from server.make_map import make_map
from server.world_makable import world_makable

pos_test_func = lambda a, b, c: True
make_map_func = make_map
world_makable_func = world_makable
