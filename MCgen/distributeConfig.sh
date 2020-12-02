#! /bin/bash

CONFIG=$1

for dr in `ls -1`; do
    if [ ! -d $dr ]; then continue; fi
    if [ "x$(echo $dr | grep pthard)" == "x" ]; then continue; fi
    cp $CONFIG $dr/
done