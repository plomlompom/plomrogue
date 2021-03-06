#!/bin/sh

# Abort the script on error.
set -e

# Run script in its own process group so SIGINT doesn't kill parent script.
set -m

# Don't let any log leftovers from before interfere.
if [ -e ./log ]
then
    rm log
fi

# Abort if no proper Python version installed.
test=`command -v python3 | wc -l`
if [ 1 -gt $test ]
then
    echo "FAILURE:"
    echo "No python3 installed, but it's needed!"
    exit 1
fi

# Give helpful message to players that want to start without compiling first.
if [ ! -e ./libplomrogue.so ]
then
    echo 'No ./libplomrogue.so library found. Building …'
    ./build.sh
fi

# Use shell script's arguments for server and pipe server output to log file.
# This script's wrapper script will read it out on exit.
python3 ./roguelike-server "$@" > log 2>&1 &

# Give server some time to start up and exit on error.
sleep 0.1

# The client should not start if the server is not running. (If the server was
# running in the foreground, any error exit of it so far would be caught by "set
# -e" above. But "set -e" is blind to error codes generated in the background.)
kill -0 $! 2> /dev/null

# Give server some time (max. 10 seconds) to generate its worldstate file.
i=0
echo -n "Waiting for server to set up world: "
while [ ! -e server_run/worldstate ] && [ $i -le 100 ]
do
    echo -n '.'
    sleep 0.1
    i=`expr $i + 1`
done
echo
if [ ! -e server_run/worldstate ]
then
    kill -9 $!
    rm -rf server_run
    echo "Server failed generating worldstate file within time limit."
    false
fi

# Only start the interface when everything else went well.
kill -0 $! 2> /dev/null
python3 ./roguelike-client
