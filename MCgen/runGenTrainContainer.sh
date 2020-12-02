#! /bin/bash

CONFIGDIR=$1
OUTPUTBASE=$2
PROCID=$3
CHUNKID=$4
ENVSCRIPT=
if [ $# -ge 5 ]; then ENVSCRIPT=$5; fi

CHUNK=$(printf "%02d" $CHUNKID)
OUTPUTDIR=$OUTPUTBASE/$CHUNK
if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi
cd $OUTPUTDIR
fls=(env.sh MLTrainDefinition.cfg globalvariables.C generator_customization.C handlers.C)
for f in ${fls[@]}; do cp $CONFIGDIR/$f $OUTPUTDIR/; done

if [ "x$ENVSCRIPT" != "x" ]; then
    echo "Running environment setup script $ENVSCRIPT" 
    source $ENVSCRIPT
fi
ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest legotrain_helpers/latest`
eval `$ALIENV list`

export ALIEN_PROC_ID=$PROCID
echo "Using ALIEN_PROC_ID $ALIEN_PROC_ID"
gencmd=$(generatetraintest &> gentraintest.log)
eval $gencmd
cmd=$(runtraintest &> train.log)
eval $cmd
rm -rf lego_train*
for f in ${fls[@]}; do rm $OUTPUTDIR/$f; done