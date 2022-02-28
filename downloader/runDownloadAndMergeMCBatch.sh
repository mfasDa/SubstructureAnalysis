#! /bin/bash

SUBSTRUCTUREREPO=$1
OUTPUTDIR=$2
DATASET=$3
TRAINRUN=$4
ALIEN_CERT=$5
ALIEN_KEY=$6

ALIENV=`which alienv`
eval `$ALIENV --no-refresh load  xjalienfs/latest`
echo `alienv list`

export PYTHONPATH=$PYTHONPATH:$SUBSTRUCTUREREPO
DOWNLOAD_EXECUTABLE=$SUBSTRUCTUREREPO/downloader/copyFromGrid.py

export ALIENPY_DEBUG_FILE=$OUTPUTDIR/alien_py.log
export JALIEN_TOKEN_CERT=$ALIEN_CERT
export JALIEN_TOKEN_KEY=$ALIEN_KEY

YEARTAG=$(echo $DATASET | cut -c4-5)
YEAR=$((YEARTAG+2000))
ALIENPATH=/alice/sim/$YEAR/$DATASET

copycmd=$(printf "%s %s %s %s" $DOWNLOAD_EXECUTABLE $ALIENPATH $TRAINRUN $OUTPUTDIR)
AODSET=$($SUBSTRUCTUREREPO/downloader/getaodset.py $DATASET)
if [ "x$(echo $AODSET)" != "xAOD" ]; then
    echo "Using AODset: $AODSET"
    copycmd=$(printf "%s -a %s" "$copycmd" "$AODSET")
else 
    echo "Using AODset: $AODSET"
fi
echo $copycmd
eval $copycmd

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