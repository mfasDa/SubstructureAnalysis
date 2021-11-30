#! /bin/bash

SUBSTRUCTURE_ROOT=$1
WORKDIR=$2
JETTYE=$3
SYSVAR=$4
MCFILE=$5

MACRO=$SUBSTRUCTURE_ROOT/unfolding/normalize1D/makeNormalizedRaw.cpp
if [ "$JETTYE" == "charged" ]; then
    MACRO=$SUBSTRUCTURE_ROOT/unfolding/normalize1D/makeNormalizedRawCharged.cpp
fi

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest root6tools/latest`
eval `$ALIENV list`

cd $WORKDIR
cmd=""
if [ "$MCFILE" != "NONE" ]; then
    echo "Full mode including trigger correction ..."
    cmd=$(printf "root -l -b -q \'%s(\"AnalysisResults.root\", \"%s\", \"%s\")\'" $MACRO $MCFILE $SYSVAR)
else
    echo "Restricted mode with data-only ..."
    cmd=$(printf "root -l -b -q \'%s(\"AnalysisResults.root\", \"\", \"%s\")\'" $MACRO $SYSVAR)
fi
echo "Running: $cmd" 
eval $cmd