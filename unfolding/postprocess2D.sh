#! /bin/bash
WORKDIR=${1:-$PWD}

export ALIBUILD_WORK_DIR=/software/markus/alice/sw
ALIENV=`which alienv`
eval `$ALIENV --no-refresh printenv AliPhysics/latest root6tools/latest`

cd $WORKDIR
ls -l
hadd -f UnfoldedSD.root UnfoldedSD_*.root
rm UnfoldedSD_*.root

#check if we have any logs not packed
if [ ! -f logs.zip ]; then
    mkdir logs
    mv *.log logs/
    zip -r logs.zip logs
    rm -rf logs
fi
