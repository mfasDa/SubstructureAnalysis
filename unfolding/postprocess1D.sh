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
        ALIBUILD_WORK_DIR_DEFAULT=/software/mfasel/alice/sw
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

fls=($(ls -1 | grep corrected | grep root | grep R))
OUTPUTFILE=
doRunMerge=1
nfls=${#fls[@]}
if [ $nfls -eq 0 ]; then 
    echo "Working directory does not contain any input file - skip merging ..."
    doRunMerge=0
else 
    # determine name of the output file
    OUTPUTFILE=$(echo ${fls[1]} | sed -e 's/\_R0[0-9]//g')
fi
if [ "x$OUTPUTFILE" != "x" ]; then
    if [ -f $OUTPUTFILE ]; then
        echo "UnfoldedSD.root existing in working directory - skip merging ..."
        doRunMerge=0
    fi
else
    echo "Could not determine output file - skip merging ..."
    doRunMerge=0
fi
if [ $doRunMerge -gt 0 ]; then
    echo "Merging to " $OUTPUTFILE
    cmd="hadd -f $OUTPUTFILE"
    for f in ${fls[@]}; do
        cmd=$(printf "%s %s" "$cmd" "$f")
    done
    eval $cmd
    for f in ${fls[@]}; do rm $f; done
fi

#check if we have any logs not packed
doCompressLogs=1
logfiles=($(ls -1 | grep log | grep -v zip))
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