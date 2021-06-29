#! /bin/bash


REPO=$1
WORKDIR=$2
RUNNUMBER=$3
MACRO=$REPO/EMCALTrigger/FastorAcceptance/findIsolated.C

ALIENV=`which alienv`
eval `$ALIENV --no-refresh load AliPhysics/latest`

cd $WORKDIR
cmd=$(printf "root -l -b -q \'%s(%d)\'" $MACRO $RUNNUMBER)
eval $cmd