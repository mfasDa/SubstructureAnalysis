#! /bin/bash
INPUTBASE=$1
OUTPUTBASE=$2
FILENAME=$3

CHUNK=$(printf "%02d" $SLURM_ARRAY_TASK_ID)
OUTPUTDIR=$OUTPUTBASE/$CHUNK
if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi
cd $OUTPUTDIR

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
eval `$ALIENV list`

ls -l $INPUTBASE
files=()
dirs=($(ls -1 $INPUTBASE))
for indir in ${dirs[@]}; do
    echo Doing $indir
    if [ "x$(echo $indir | grep LHC)" == "x" ]; then continue; fi
    fname=$INPUTBASE/$indir/merged/$CHUNK/$FILENAME
    echo Doing Filename $fname
    if [ -f $fname ]; then
        echo Adding input file $fname
        files+=($fname)
    fi
done

hadd -f $FILENAME ${files[@]}