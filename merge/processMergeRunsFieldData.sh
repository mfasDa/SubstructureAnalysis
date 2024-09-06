#! /bin/bash

REPO=$1
INPUTDIR=$2
ROOTFILENAME=$3
FIELD=$4

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`

if [ "x$(echo $SUBSTRUCTURE_ROOT)" == "x" ]; then
    source $REPO/env.sh
fi

EXE=$REPO/merge/mergeRunsField.py
cmd=$(printf "%s -i %s -f %s -b %s" $EXE $INPUTDIR $ROOTFILENAME $FIELD)
eval $cmd