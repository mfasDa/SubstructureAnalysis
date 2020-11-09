#! /bin/bash

FILE1=$1
FILE2=$2
OUTPUTFILE=$3

echo FILE1 $FILE1
echo FILE2 $FILE2
echo OUTPUTFILE $OUTPUTFILE

ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest`
hadd -f $OUTPUTFILE $FILE1 $FILE2