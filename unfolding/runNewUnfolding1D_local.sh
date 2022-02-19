#! /bin/bash

REPOSITORY=$1
WORKDIR=$2
DATAFILE2017=$3
DATAFILE2018=$3
MCFILE=$4
SYSVAR=$5
UNFOLDINGMACRO=$6

RADIUS=$SLURM_ARRAY_TASK_ID

if [ "x$(echo $CLUSTER)" == "xCADES" ]; then
    source /home/mfasel_alice/alice_setenv
fi

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
eval `$ALIENV list`

SOURCEDIR=$REPOSITORY/unfolding/1D
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
cmd=$(printf "root -l -b -q \'%s(\"%s\", \"%s\", \"%s\", \"%s\", %d)\' &> %s/unfolding_R%02d.log" $SCRIPT $DATAFILE2017 $DATAFILE2018 $MCFILE $SYSVAR $RADIUS $WORKDIR $RADIUS)
echo Running command: $cmd
eval $cmd