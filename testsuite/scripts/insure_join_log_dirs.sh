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

# delete profiles with size 0
find $JOINEDDIR -name "tca.*.log" -size 0 -exec rm {} \;
exit 0
