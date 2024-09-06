#! /bin/bash

INPUTBASE=$1
OUTPUTBASE=$2
FILENAME=$3
FIELD=$4
REPO=$5

CHUNK=$(printf "%02d" $SLURM_ARRAY_TASK_ID)
INPUTDIR=$INPUTBASE/$CHUNK
OUTPUTDIR=$OUTPUTBASE/$CHUNK
if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi
cd $OUTPUTDIR

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
eval `$ALIENV list`

if [ "x$(echo $SUBSTRUCTURE_ROOT)" == "x" ]; then
    source $REPO/env.sh
fi

EXE=$REPO/merge/mergeRunsField.py
cmd=$(printf "%s -i %s -o %s -f %s -b %s" $EXE $INPUTDIR $OUTPUTDIR $FILENAME $FIELD)
echo $cmd
eval $cmd
