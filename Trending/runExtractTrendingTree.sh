#! /bin/bash

SOURCEDIR=$1
WORKDIR=$2
RUNNUMBER=$3

TRENDINGMACRO=$SOURCEDIR/Trending/trendingExtractorData.C
ALIENV=`which alienv`
export ALIBUILD_WORK_DIR=/software/mfasel/alice/sw
eval `$ALIENV --no-refresh printenv AliPhysics/latest`

cd $WORKDIR
cmd=$(printf "root -l -b -q \'%s(\"AnalysisResults.root\", %d)\'" $TRENDINGMACRO $RUNNUMBER)
eval $cmd