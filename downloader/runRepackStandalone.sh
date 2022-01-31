#! /bin/bash
OUTPUTDIR=$1
# re-unpack data
pthbins=($(ls -1 $OUTPUTDIR))
base=$PWD
for pth in ${pthbins[@]}; do
    pthdir=$OUTPUTDIR/$pth
    if [ ! -d $pthdir ]; then continue; fi
    runs=($(ls -1 $pthdir))
    for run in ${runs[@]}; do
        rundir=$pthdir/$run
        if [ ! -d $rundir ]; then continue; fi
        cd $rundir
        if [ -f root_archive.zip ]; then
            rm -vf *.root
            unzip root_archive.zip
        fi
    done
done