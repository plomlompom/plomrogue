#!/bin/sh

# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.

set -e

OFLAG='-O3'
CFLAGS="-std=c11 -pedantic-errors -Wall -Werror -Wextra -Wformat-security $OFLAG"

# For non-GNU gcc masks, drop all gcc-specific debugging flags.
test=`stat --version 2>&1 | grep 'Free Software Foundation' | wc -l`
if [ 1 -gt $test ]
then
    CFLAGS=$OFLAG
fi

# Some tests: for gcc, and certain necessary header files.
test=`command -v gcc | wc -l`
if [ 1 != $test ]
then
    echo "FAILURE:"
    echo "No gcc installed, but it's needed!"
    exit 1
fi
test_header() {
    code="#include <$1>"
    test=`echo $code | cpp -H -o /dev/null 2>&1 | head -n1 | grep error | wc -l`
    if [ 0 != $test ]
    then
        echo "FAILURE:"
        echo "No $1 header file found, but it's needed!"
        echo "Maybe install some $2 package?"
        exit 1
    fi
}
test_header stdlib.h libc6-dev      # Assume stdlib.h guarantees full libc6-dev.

# Compilation proper.
gcc -shared -fPIC $CFLAGS -o libplomrogue.so libplomrogue.c -lm
