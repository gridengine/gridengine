#!/bin/sh
#$ -S /bin/sh

usage() {
   echo "infinity.sh lck_file"
   exit 1
}


if [ $# -ne 1 ]; then
   usage
fi

lck_file=$1

if [ -f $lck_file ]; then
   echo "lock file $lck_file already exists"
   exit 3
fi

touch $lck_file
res=$?
if [ $res != 0 ]; then
   echo "Can not create lock file $lck_file (error $res)"
   exit 4
fi

trap 'echo "got signal TERM"; rm -f $lck_file' TERM
trap 'echo "got signal SIGUSR1, job will be deleted soon"' USR1
trap 'echo "got signal SIGUSR2, job will be deleted soon"' USR2


while [ -f $lck_file ]; do
   echo "sleep"
   sleep 10   
done

echo "finished"


