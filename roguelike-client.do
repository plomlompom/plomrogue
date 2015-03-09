# redo build file to build the executable "roguelike-client".

# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.

redo-ifchange build/compiler_flags
. ./build/compiler_flags
mkdir -p build/client
mkdir -p build/common
for file in src/client/*.c src/common/*.c; do
  file=build/${file#src/}
  redo-ifchange ${file%.*}.o
done
gcc $CFLAGS -o $3 -g build/client/*.o build/common/*.o -lncurses
