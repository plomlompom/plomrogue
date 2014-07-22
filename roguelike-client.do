# redo build file to build the executable "roguelike-client".

redo-ifchange build/build_template
TARGET=client
LIBRARY_LINKS=-lncurses
. ./build/build_template
