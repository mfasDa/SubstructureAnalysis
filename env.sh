#! /bin/bash

SCRIPTNAME=`readlink -f $0`
export SUBSTRUCTURE_ROOT=`dirname $SCRIPTNAME`
export PYTHONPATH=$PYTHONPATH:$SUBSTRUCTURE_ROOT
