#!/bin/sh
#$ -S /bin/sh
#$ -N "My Name"
if [ "$JOB_NAME" != "My Name" ]
then exit 1
else exit 0
fi