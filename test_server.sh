#!/bin/sh

./redo
cp ./roguelike-server ./testing/tested_server
cp ./src/server/libplomrogue.c ./testing/tested_server_lib.c

cp ./testing/start _test
echo "Starting server on save file '_test' copied from ./testing/start."
python3 -m cProfile -o ./testing/cProfile roguelike-server -l _test &

echo "Waiting until ./server/in is ready for writing."
while [ ! -e ./server/in ]
do
    sleep 0.1
done

echo "Catting ./testing/run into ./server/in."
cat ./testing/run >> server/in

echo "Waiting until server has quit (i.e. ./server/in has disappeared)."
while [ -e ./server/in ]
do
    sleep 1
done

echo "Server has quit. Here's the diff of final '_test' to reference save file"
echo "./testing/ref_end (TEST WENT WELL IF NO LINES FOLLOW):"
mv _test testing/last_end
diff testing/last_end testing/ref_end
#echo 'NODIFF'

rm record__test
