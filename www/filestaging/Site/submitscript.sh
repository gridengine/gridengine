#!/bin/sh

# script to do word counts of Project Gutenberg texts

if [ $#ARGV -ne 2 ]; then
	echo "Usage: <subdirectory of pub/gutenberg> <filename>"
	echo "Some example inputs:"
	echo "$0 etext00 00ws110.txt"
	echo "$0 etext00 01frd10.txt"
	echo "$0 etext01 ironm10.txt"
	exit 1
fi

DIR=$1
FILE=$2

# note: use timestamp to ensure job has unique name
#
STAMP=`/usr/bin/date +%d%H%M`.$$.$JOB_ID

# first submit the transfer job.  You might want to use a Boolean resource
# to make this job run on hosts which are more suitable for this task, eg,
# because they have a fast connection to the Internet or are able to go
# outside the firewall
#
qsub -N transjob.$STAMP ./transferjob.sh ftp.cdrom.com pub/gutenberg/$DIR $FILE /array1/Projects/Filestaging

# submit the counting job with a dependency so it only runs
# when the transfer job is complete
#
qsub -o word_report_$FILE -j y -hold_jid transjob.$STAMP countword.pl /array1/Projects/Filestaging/$FILE

