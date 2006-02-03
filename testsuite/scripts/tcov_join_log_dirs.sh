#!/bin/sh

BASEDIR=$1
JOINEDDIR=$2

if [ ! -d $BASEDIR ]; then
   echo "No coverage information found"
   exit 1
fi

if [ ! -d $JOINEDDIR ]; then
   mkdir -p $JOINEDDIR
fi

cd $BASEDIR
cp -rp * $JOINEDDIR
exit 0
