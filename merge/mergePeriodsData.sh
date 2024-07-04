#! /bin/bash

DATADIR=$1

export ALIBUILD_WORK_DIR=/software/mfasel/alice/sw
ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
$ALIENV list

echo Finding periods in $DATADIR ...
periods=($(ls -1 $DATADIR | grep "LHC"))
echo Found ${#periods[@]} periods ...

RESULTDIR=$DATADIR/merged
if [ ! -d $RESULTDIR ]; then mkdir -p $RESULTDIR; fi

cmd="hadd -f $RESULTDIR/AnalysisResults.root"
for period in ${periods[@]}; do
    periodfile=$DATADIR/$period/merged/AnalysisResults.root
    if [ -f $periodfile ]; then
        echo "Adding $periodfile"
        cmd=$(printf "%s %s" "$cmd" "$periodfile")
    else
        echo "$periodfile not found"
    fi
done

echo "Doing $cmd"
eval $cmd
echo "Done ..."