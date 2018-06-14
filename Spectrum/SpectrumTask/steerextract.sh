#! /bin/bash

SCRIPTNAME=`readlink -f $0`
SCRIPTDIR=`dirname $SCRIPTNAME`
EXTRACTMACRO=$SCRIPTDIR/extractJetSpectrum.cpp

BASEDIR=`pwd`
runs=($(ls -1 $BASEDIR | grep -v [A-Z] | grep -v [a-z]))
for r in ${runs[@]}; do
  cd $r
  cmd=$(printf "root -l -b -q %s" $EXTRACTMACRO)
  eval $cmd
  cd $BASEDIR
done
