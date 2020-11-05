#! /bin/bash

INPUTBASE=$1
OUTPUTBASE=$2
FILENAME=$3

CHUNK=$(printf "%02d" $SLURM_ARRAY_TASK_ID)
INPUTDIR=$INPUTBASE/$CHUNK
OUTPUTDIR=$OUTPUTBASE/$CHUNK
if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi
cd $OUTPUTDIR

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
eval `$ALIENV list`
hadd -f $FILENAME $INPUTDIR/*/$FILENAME