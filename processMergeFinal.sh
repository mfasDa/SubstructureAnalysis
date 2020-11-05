#! /bin/bash

INPUTDIR=$1
FILENAME=$2

cd $INPUTDIR

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
eval `$ALIENV list`
hadd -f $INPUTDIR/$FILENAME $INPUTDIR/*/$FILENAME