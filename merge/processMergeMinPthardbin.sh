#! /bin/bash

WORKDIR=$1
MINPTHARD=$2
FILENAME=$3

ALIENV=`which alienv`
eval ``

FILEBASE=$(echo $FILENAME | sed -e 's/\.root//g')
OUTPUTFILE=$(printf "%s_minpthard%d.root" $FILEBASE $MINPTHARD)

cd $WORKDIR
cmd="hadd -f $OUTPUTFILE"
for fl in `seq $MINPTHARD 20`; do
    ADDFILE=$(printf " %02d/%s" $FILENAME)
    cmd=$(printf "%s %s" "$cmd" $ADDFILE)
done
eval $cmd