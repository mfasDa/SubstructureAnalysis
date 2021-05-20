#! /bin/bash

SUBSTRUCTUREREPO=$1
ALIEN_CERT=$2
ALIEN_KEY=$3
OUTPUTDIR=$4
TRAINRUN=$5
LEGOTRAIN=$6
DATASET=$7
PASS=$8
AODPROD=$9
FILENAME="${10}"

echo "Dataset:            $DATASET"
echo "Trainrun:           $TRAINRUN"
echo "Legotrain:          $LEGOTRAIN"
echo "Pass:               $PASS"
echo "AOD production:     $AODPROD"
echo "Filename:           $FILENAME"

ALIENV=`which alienv`
eval `$ALIENV --no-refresh load AliPhysics/latest`
$ALIENV list

export PYTHONPATH=$PYTHONPATH:$SUBSTRUCTUREREPO
DOWNLOAD_EXECUTABLE=$SUBSTRUCTUREREPO/downloader/copyTrainRunwise.py

export ALIENPY_DEBUG_FILE=$OUTPUTDIR/alien_py.log
export JALIEN_TOKEN_CERT=$ALIEN_CERT
export JALIEN_TOKEN_KEY=$ALIEN_KEY

cd $OUTPUTDIR
copycmd=$(printf "%s %s %s %s %s" $DOWNLOAD_EXECUTABLE $OUTPUTDIR $TRAINRUN $LEGOTRAIN $DATASET)
if [ "$PASS" != "NONE" ]; then
    copycmd=$(printf "%s -p %s" "$copycmd" $PASS)
fi
if [ "$AODPROD" != "NONE" ]; then
    copycmd=$(printf "%s -a %s" "$copycmd" $AODPROD)
fi
if [ "$FILENAME" != "NONE" ]; then
    copycmd=$(printf "%s -f %s" "$copycmd" $FILENAME)
fi
echo Running $copycmd
eval $copycmd