#!/bin/sh

# Abort the script on error.
set -e

# Don't let any log leftovers from before interfere.
if [ -e ./log ]
then
    rm log
fi

# Give helpful message to players that want to start without compiling first.
if [ ! -e ./roguelike-server ]
then
    echo 'No ./roguelike-server file found to execute. Try "make" first?'
    false
fi
if [ ! -e ./roguelike-client ]
then
    echo 'No ./roguelike-client file found to execute. Try "make" first?'
    false
fi

# Use shell script's arguments for server and pipe server output to log file.
# This script's wrapper script will read it out on exit.
./roguelike-server "$@" > log 2>&1 &

# Give server some time to start up and exit on error.
sleep 0.01

# The client should not start if the server is not running. (If the server was
# running in the foreground, any error exit of it so far would be caught by "set
# -e" above. But "set -e" is blind to error codes generated in the background.)
kill -0 $! 2> /dev/null

# Give server some time (max. 10 seconds) to generate its worldstate file.
i=0
while [ ! -e server/worldstate ] && [ $i -le 1000 ]
do
    sleep 0.01
    i=`expr $i + 1`
done
if [ ! -e server/worldstate ]
then
    echo "Server failed generating worldstate file within given time limit."
    false
fi

# Only start the interface when everything else went well.
kill -0 $! 2> /dev/null
./roguelike-client
