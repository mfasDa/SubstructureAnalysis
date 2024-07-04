#! /bin/bash

REPO=$1
INPUTDIR=$2
OUTPUTDIR=$3
PTHARDBIN=$SLURM_ARRAY_TASK_ID
INPUTFILE=$INPUTDIR/$(printf "%02d" $PTHARDBIN)/AnalysisResults.root

SCRIPT=$REPO/omnifold/extractResponse.py

ALIENV=`which alienv`
export ALIBUILD_WORK_DIR=/software/mfasel/alice/sw
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
alienv list

if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi

$SCRIPT $INPUTFILE -o $OUTPUTDIR

echo Done ...
