#! /bin/bash
INPUTBASE=$1
OUTPUTBASE=$2
FILENAME=$3
MERGEDIR=$4

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
    if [ ! -d $INPUTBASE/$indir ]; then continue; fi
    # do not explicitly require a period tag (won't work for runwise output),
    # but rather only veto the merged outputs
    if [ "x$(echo $indir | grep merged)" != "x" ]; then continue; fi
    fname=
    if [ -d $INPUTBASE/$indir/$MERGEDIR ]; then
        # sample-wise output usually contains a merged directory - use that
        fname=$INPUTBASE/$indir/$MERGEDIR/$CHUNK/$FILENAME
    else
        # runwise output doesn't have a merged directory (because there is no 2-step merging per sample)
        # instead it has to use the input directory of the sample (run) directly
        fname=$INPUTBASE/$indir/$CHUNK/$FILENAME
    fi
    echo Doing Filename $fname
    if [ -f $fname ]; then
        echo Adding input file $fname
        files+=($fname)
    fi
done

hadd -f $FILENAME ${files[@]}