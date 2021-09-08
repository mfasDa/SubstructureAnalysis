#! /bin/bash

INPUTDIR=$1
FILENAME=$2
SUBSTRUCTURE_ROOT=$3
RUNCHECK=$4

cd $INPUTDIR

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
eval `$ALIENV list`
hadd -f $INPUTDIR/$FILENAME $INPUTDIR/*/$FILENAME
if [ $RUNCHECK -gt 0 ]; then
	echo "Running check of the pt-hard sample"
	MACRO=$SUBSTRUCTURE_ROOT/unfolding/checkPtHardSample.C
	cmd=$(printf "root -l -b -q \'%s\'" $MACRO)
	eval $cmd
fi