# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.

redo-ifchange compiler_flags
. ./compiler_flags
file=${1#build/}
gcc $CFLAGS -o $3 -c ../src/${file%.o}.c -MD -MF $2.deps
read DEPS <$2.deps
redo-ifchange ${DEPS#*:}
