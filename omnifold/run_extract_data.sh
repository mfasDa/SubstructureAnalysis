#! /bin/bash

REPO=$1
INPUTDIR=$2
OUTPUTDIR=$3
TAG=$4
INPUTFILE=$INPUTDIR/AnalysisResults.root

SCRIPT=$REPO/omnifold/extractData.py

ALIENV=`which alienv`
export ALIBUILD_WORK_DIR=/software/mfasel/alice/sw
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
alienv list

if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi

cmd=$(printf "%s %s %s" "$SCRIPT" "$INPUTFILE" -o "$OUTPUTDIR")
if [ "x$TAG" != "xNONE" ]; then
    cmd=$(printf "%s -t %s" "$cmd" "$TAG")
fi
echo Running $cmd
eval $cmd

echo Done ...
