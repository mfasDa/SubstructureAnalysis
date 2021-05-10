#! /bin/bash

SUBSTRUCTUREREPO=$1
INPUTDIR=$2
OUTPUTDIR=$3
ROOTFILE=$4
RUNWISE=$5

SCRIPT=$SUBSTRUCTUREREPO/downloader/sort_periodwise.py
if [ $RUNWISE -gt 0 ]; then
    SCRIPT=$SUBSTRUCTUREREPO/downloader/sort_runwise.py
fi

cmd=$(printf "%s -i %s -o %s -r %s" $SCRIPT $INPUTDIR $OUTPUTDIR $ROOTFILE)
eval `$cmd`