#!/bin/sh

usage() {
  echo "termtest.sh host job_owner job_id job_name queue job_pid log_file"
  exit 1
}
if [ $# -ne 7 ]; then
   usage
fi

host=$1
job_owner=$2
job_id=$3
job_name=$4
queue=$5
job_pid=$6
log_file=$7

date=`date '+%y-%m-%d %H:%M:%S'`



printf "%s: Terminate job %s (name %s, pid %s) on queue %s: -> " \
       $date $job_id $job_name $job_pid $queue >> $log_file
       
kill $job_pid
res=$?

if [ $res -eq 0 ]; then
  echo "Killed"  >> $log_file
  exit 0
else
  echo "Error $res" >> $log_file
  exit 1
fi


