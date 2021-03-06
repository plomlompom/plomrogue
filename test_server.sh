#!/bin/sh

set -e

./build.sh
echo "Copying tested server to ./testing/tested_server.py and ./testing/server."
cp ./roguelike-server ./testing/tested_server.py
cp -R ./server ./testing/
echo "Copying tested C library source to ./tested_server_lib.c."
cp ./libplomrogue.c ./testing/tested_server_lib.c

cp ./testing/start _test
echo "Starting server on save file '_test' copied from ./testing/start."
python3 -m cProfile -o ./testing/cProfile roguelike-server -l _test &

echo "Waiting until ./server/in is ready for writing."
while [ ! -e ./server_run/in ]
do
    sleep 0.1
done

echo "Catting ./testing/run into ./server/in."
cat ./testing/run >> server_run/in

echo "Waiting until server has quit (i.e. ./server/in has disappeared)."
while [ -e ./server_run/in ]
do
    sleep 1
done

echo "Server has quit. Here's the length of the diff of final '_test' (copy at"
echo "./testing/last_end) to the reference save file ./testing/ref_end (TEST WENT "
echo "WELL IF ZERO): "
mv _test testing/last_end
diff testing/last_end testing/ref_end | wc -l

rm record__test
