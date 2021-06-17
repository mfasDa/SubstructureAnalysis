#! /bin/bash

WORKDIR=$1
DATAFILE=$2
MCFILE=$3
SYSVAR=$4
UNFOLDINGMACRO=$5

RADIUS=$SLURM_ARRAY_TASK_ID

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
eval `$ALIENV list`

SOURCEDIR=/software/markus/alice/SubstructureAnalysis/unfolding/1D
UNFOLDINGMETHOD=""
if [ "x$(echo $UNFOLDINGMACRO | grep SVD)" != "x" ]; then
    UNFOLDINGMETHOD="SVD"
elif [ "x$(echo $UNFOLDINGMACRO | grep Bayes)" != "x" ]; then
    UNFOLDINGMETHOD="Bayes"
else
    echo "Unknown unfolding method, skipping ..."
    exit 1
fi
echo "Using unfolding method $UNFOLDINGMETHOD"
SCRIPT=$SOURCEDIR/$UNFOLDINGMETHOD/$UNFOLDINGMACRO
if [ ! -f $SCRIPT ]; then
    echo Unfolding macro $SCRIPT does not exist, skipping ...
    exit 1
fi
echo "Using unfolding macro $SCRIPT"

if [ ! -d $WORKDIR ]; then mkdir -p $WORKDIR; fi
cd $WORKDIR
cmd=$(printf "root -l -b -q \'%s(\"%s\", \"%s\", \"%s\", %d)\' &> %s/unfolding_R%02d.log" $SCRIPT $DATAFILE $MCFILE $SYSVAR $RADIUS $WORKDIR $RADIUS)
echo Running command: $cmd
eval $cmd