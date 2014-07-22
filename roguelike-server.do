# redo build file to build the executable "roguelike-server".

redo-ifchange build/build_template
TARGET=server
. ./build/build_template
