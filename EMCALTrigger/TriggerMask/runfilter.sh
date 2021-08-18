#! /bin/bash

REPO=$1
OUTPUTBASE=$2
RUNNUMBER=$3

ALIENV=`which alienv`
eval `$ALIENV --no-refresh load AliPhysics/latest`
eval $ALIENV list

ENVSCRIPT=$REPO/env.sh
if [ -d $ENVSCRIPT ]; then source $ENVSCRIPT; fi

RUNDIR=$OUTPUTBASE/$RUNNUMBER
if [ ! -d $RUNDIR ]; then mkdir -p $RUNDIR; fi
cd $RUNDIR
CMD=$(printf "root -l -b -q \'%s/EMCALTrigger/TriggerMask/extractMaskForRun.C(%d)\' &> extract.log" $SUBSTRUCTURE_ROOT $RUNNUMBER)
eval $CMD