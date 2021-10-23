#! /bin/bash

REPOSITORY=$1
OUTPUTDIR=$2
DATAFILE=$3
MCFILE=$4
OBSERVABLE=$5
BINVAR=$6
RSTRING=$7
DOEFFPURE=$8

if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi
cd $OUTPUTDIR

export ALIBUILD_WORK_DIR=/software/mfasel/alice/sw
eval `alienv --no-refresh printenv AliPhysics/latest-ali-master-root6`
UNFOLDINGMACRO=$REPOSITORY/unfolding/runUnfolding2D_FromFile.C
LOGFILE=unfolding_$OBSERVABLE\_$RSTRING\.log
CORRSTRING=
if [ $DOEFFPURE -gt 0 ]; then
    CORRSTRING=kTRUE
else 
    CORRSTRING=kFALSE
fi
CMD=$(printenv "root -l -b -q \'%s(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %s)\' &> %s" $UNFOLDINGMACRO $DATAFILE $MCFILE $OBSERVABLE $BINVAR $RSTRING $CORRSTRING $LOGFILE)
eval $CMD
echo "Done ..."