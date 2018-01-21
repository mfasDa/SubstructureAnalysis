#! /bin/bash

UNFOLDINGMACRO=$1
OUTPUTDIR=$2
DATAFILE=$3
MCFILE=$4

if [ ! -d $OUTPUTDIR ]; then mkdir -p $OUTPUTDIR; fi
cd $OUTPUTDIR

myalibash AliPhysics/latest-ali-work-root6 
myalibash RooUnfold/latest
myalienv list

cmd=$(printf "root -l -b -q \'%s(\"%s\", \"%s\")\' >> unfolding.log" $UNFOLDINGMACRO $DATAFILE $MCFILE)
echo $cmd
eval $cmd

echo "Done ..."