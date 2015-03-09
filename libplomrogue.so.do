# redo build file to build the library "libplomrogue.so"

# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.

redo-ifchange build/compiler_flags
. ./build/compiler_flags
redo-ifchange src/server/libplomrogue.c
gcc -shared -fPIC $CFLAGS -o libplomrogue.so src/server/libplomrogue.c -lm
