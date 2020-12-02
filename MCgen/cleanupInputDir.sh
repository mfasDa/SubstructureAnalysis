#! /bin/bash

BASE=$PWD
INPUTDIR=$1
cd $INPUTDIR
tookeep=(env.sh MLTrainDefinition.cfg globalvariables.C generator_customization.C handlers.C)
for f in `ls -1`; do
    found=0
    for t in ${tookeep[@]}; do
        if [ $f == $t ]; then found=1; break; fi 
    done
    if [ $found -eq 0 ]; then rm $f; fi
done
cd $BASE
