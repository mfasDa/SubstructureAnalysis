#! /bin/bash

REPOSITORY=$1
OUTPUTDIR=$2
DATAFILE=$3
MCFILE=$4
OBSERVABLE=$5
RSTRING=$6

if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi
cd $OUTPUTDIR

export ALIBUILD_WORK_DIR=/clusterfs1/markus/alice/sw
eval `alienv --no-refresh printenv AliPhysics/latest-ali-master-root6`
UNFOLDINGMACRO=$REPOSITORY/unfolding/runUnfolding2D_FromFile.C
LOGFILE=unfolding_$OBSERVABLE\_$RSTRING\.log
CMD=$(printenv "root -l -b -q \'%s(\"%s\", \"%s\", \"%s\", \"%s\")\' &> %s" $UNFOLDINGMACRO $DATAFILE $MCFILE $OBSERVABLE $RSTRING $LOGFILE)
eval $CMD
echo "Done ..."