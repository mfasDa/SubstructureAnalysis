#! /bin/bash

SCRIPTBASE=`dirname $0`
SOURCEDIR=`readlink -f $SCRIPTBASE`
INPUTDIR=$PWD

for r in `seq 0 20`; do
  chunkdir=$(printf "%02d" $r)
  if [ ! -d $chunkdir ]; then continue; fi
  cd $chunkdir
  for f in `ls -1 | grep JetSubstructureTree | grep -v filtered`; do
    echo "Filtering $f from chunk $chunkdir"
    cmd=$(printf "root -l -b -q \'%s/FilterTree.cpp(\"%s\")\'" $SOURCEDIR $f)
    eval $cmd
  done
  cd $INPUTDIR
done