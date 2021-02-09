#! /bin/bash
WORKDIR=${1:-$PWD}

packages=()
if [ "x$ALICE_PHYSICS" == "x" ]; then
    echo "Package AliPhysics not yet loaded - adding"
    packages+=("AliPhysics")
fi
if [ "x$ROOT6TOOLS_ROOT" == "x" ]; then
    echo "Package root6tools not yet loaded - adding"
    packages+=("root6tools")
fi
if [ ${#packages[@]} -gt 0 ]; then
    echo "Missing packages, load"
    if [ "x$ALIBUILD_WORK_DIR" == "x" ]; then
        ALIBUILD_WORK_DIR_DEFAULT=/software/markus/alice/sw
        echo "Setting ALIBUILD_WORK_DIR to $ALIBUILD_WORK_DIR_DEFAULT"
        export ALIBUILD_WORK_DIR=$ALIBUILD_WORK_DIR_DEFAULT
    fi
    ALIENV=`which alienv`
    for package in ${packages[@]}; do
        echo "Loading package $package"
        eval `$ALIENV --no-refresh load $package/latest`
    done
fi

cd $WORKDIR
ls -l
# protect existing files from being overwriten

doRunMerge=1
if [ -f UnfoldedSD.root ]; then
    echo "UnfoldedSD.root existing in working directory - skip merging ..."
    doRunMerge=0
fi
fls=($(ls -1 | grep UnfoldedSD_))
nfls=${#fls[@]}
if [ $nfls -eq 0 ]; then 
    echo "Working directory does not contain any input file - skip merging ..."
    doRunMerge=0
fi
if [ $doRunMerge -gt 0 ]; then
    hadd -f UnfoldedSD.root UnfoldedSD_*.root
    rm UnfoldedSD_*.root
fi

#check if we have any logs not packed
doCompressLogs=1
logfiles=($(ls -1 | grep log | grep unfolding | grep -v zip))
nlogs=${#logfiles[@]}
if [ $nlogs -eq 0 ]; then
    echo "Not packing logs because no logfile has been found"
    doCompressLogs=0
fi
if [ -f logs.zip ]; then
    if [ $nlogs -gt 0 ]; then
        archives=($(ls -1 | grep log | grep zip))
        narchives=${#archives[@]}
        echo "Moving old log archive to log.$narchives.zip as new logs have been found"
        mv logs.zip logs.$narchives.zip
    fi
fi
if [ $doCompressLogs -gt 0 ]; then
    mkdir logs
    mv *.log logs/
    zip -r logs.zip logs
    rm -rf logs
fi