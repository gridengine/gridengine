#!/bin/sh

Usage()
{
   echo "monitor.sh [options]"
   echo "options:"
   echo "   -cell       <cell>     use \$SGE_CELL other than \"default\""
   echo "   -interval   <time>     use statistics interval other than \"15sec\""
   echo "   -spooling              show qmaster spooling probes"
   echo "   -requests              show incoming qmaster request probes"
   echo "   -verify                do probe verifcation only"
}

# monitor.sh defaults
cell=default
interval=15sec
spooling_probes=0
request_probes=0
verify=0

while [ $# -gt 0 ]; do
   case "$1" in
      -verify)
         verify=1
         shift
         ;;
      -spooling)
         spooling_probes=1
         shift
         ;;
      -requests)
         request_probes=1
         shift
         ;;
      -interval)
         shift
         interval="$1"
         shift
         ;;
      -cell)
         shift
         cell="$1"
         shift
         ;;
      -help)
         Usage
         exit 0
         ;;
      *)
         Usage
         exit 1
         ;;
   esac
done

if [ $SGE_ROOT = "" ]; then
   echo "Please run with \$SGE_ROOT set on master machine"
   exit 1
fi

qmaster_spool_dir=`grep '^qmaster_spool_dir' $SGE_ROOT/$cell/common/bootstrap|awk '{ print $2 }'`
master=`cat $qmaster_spool_dir/qmaster.pid`
if [ $? -ne 0 ]; then
   echo "Couldn't read sge_qmaster pid from \$SGE_ROOT/$cell/spool/qmaster/qmaster.pid"
   exit 1
fi
schedd=`cat $qmaster_spool_dir/schedd/schedd.pid`
if [ $? -ne 0 ]; then
   echo "Couldn't read sge_schedd pid from \$SGE_ROOT/$cell/spool/qmaster/schedd/schedd.pid"
   exit 1
fi

/usr/sbin/dtrace -s ./monitor.d $master $schedd $interval $spooling_probes $request_probes $verify
