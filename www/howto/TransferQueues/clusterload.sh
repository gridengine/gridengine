#!/bin/sh

# load sensor to report number of running jobs on a cluster
# NOTE: make sure the host which this script runs on is a submit host
# for the remote cluster
# below should contain path to SGE_ROOT of cluster being queried

SGE_ROOT=/net/tcf27-b030/share/gridware
export SGE_ROOT

ARCH=`$SGE_ROOT/util/arch`

PATH=/bin:/usr/bin:$SGE_ROOT/bin/$ARCH

end=false
while [ $end = false ]; do

   jobs=`qstat -s p -g d | awk '$1 ~ /^[0-9]/ {print $0} '| wc -l`

   # ---------------------------------------- 
   # wait for an input
   #
   read input
   if [ $? != 0 ]; then
      end=true
      break
   fi
   
   if [ "$input" = "quit" ]; then
      end=true
      break
   fi

   echo "begin"
   echo "global:mpk27jobs:$jobs"
   echo "end"

done
