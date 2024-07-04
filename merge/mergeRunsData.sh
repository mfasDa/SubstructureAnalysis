#! /bin/bash

PERIODDIR=$1

export ALIBUILD_WORK_DIR=/software/mfasel/alice/sw
ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
$ALIENV list

echo Finding runs in $PERIODDIR ...
runs=($(ls -1  $PERIODDIR| grep -v [a-z,A-Z]))
echo Found ${#runs[@]} runs ...

RESULTDIR=$PERIODDIR/merged
if [ ! -d  $RESULTDIR ]; then mkdir -p $RESULTDIR; fi

cmd="hadd -f $RESULTDIR/AnalysisResults.root"
for rdir in ${runs[@]}; do
    rfile=$PERIODDIR/$rdir/AnalysisResults.root
    if [ -f $rfile ]; then
        echo "Adding $rfile"
        cmd=$(printf "%s %s" "$cmd" "$rfile")
    else
        echo "$rfile not found"
    fi
done

echo "Running $cmd"
eval $cmd
echo "Done ..."