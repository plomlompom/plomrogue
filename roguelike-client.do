# redo build file to build the executable "roguelike-client".

# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.

redo-ifchange build/build_template
TARGET=client
LIBRARY_LINKS=-lncurses
. ./build/build_template
