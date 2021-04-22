$ cat downloadAndMergeData.sh
#! /bin/bash

DOWNLOADREPO=$1
OUTPUTDIR=$2
YEAR=$3
TRAINRUN=$4
ALIEN_CERT=$5
ALIEN_KEY=$6

DOWNLOAD_EXECUTABLE=$DOWNLOADREPO/download$YEAR.py

ALIENV=`which alienv`
eval `$ALIENV --no-refresh load AliPhysics/latest`
$ALIENV list

export ALIENPY_DEBUG_FILE=$OUTPUTDIR/alien_py.log
export JALIEN_TOKEN_CERT=$ALIEN_CERT
export JALIEN_TOKEN_KEY=$ALIEN_KEY

cd $OUTPUTDIR
copycmd=$(printf "%s %s" $DOWNLOAD_EXECUTABLE $TRAINRUN)
echo Running $copycmd
eval $copycmd

mkdir merge
hadd -f merge/AnalysisResults.root LHC*/AnalysisResults.root