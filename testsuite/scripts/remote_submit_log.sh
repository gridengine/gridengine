#!/bin/sh
#___INFO__MARK_BEGIN__
##########################################################################
#
#  The Contents of this file are made available subject to the terms of
#  the Sun Industry Standards Source License Version 1.2
#
#  Sun Microsystems Inc., March, 2001
#
#
#  Sun Industry Standards Source License Version 1.2
#  =================================================
#  The contents of this file are subject to the Sun Industry Standards
#  Source License Version 1.2 (the "License"); You may not use this file
#  except in compliance with the License. You may obtain a copy of the
#  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
#
#  Software provided under this License is provided on an "AS IS" basis,
#  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
#  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
#  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
#  See the License for the specific provisions governing your rights and
#  obligations concerning the Software.
#
#  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
#
#  Copyright: 2001 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__


# par1: product root
# par2: cell
# par2: submit util this job id is existent
# par3: protocol dir
# par4: sleep seconds time list
# par5: optional - jobscript to submit
# par6: optional - second parameter for jobscript
. $1/$2/common/settings.sh

arch=`$1/util/arch`
jobscript=$1/examples/jobs/sleeper.sh
touchdir=""
if [ $# -ge 6 ]; then
   jobscript=$6
   touchdir=$5
fi

stop_job_id=$3
times=$5

hostname=`$1/utilbin/$arch/gethostname -name | cut -f 1 -d .`

logfile="$4/${hostname}.submit.log"
jobs_submitted_file="$4/${hostname}.submitted"

if [ -f $logfile ]; then
   mv $logfile $logfile.`date +20%y%m%d_%H-%M-%S`
fi

if [ -f $jobs_submitted_file ]; then
   rm $jobs_submitted_file
fi

echo "$hostname" > $logfile
echo "0" > $jobs_submitted_file

submitted=0
do_submit=1
act_job_id=0

testruns=10
while [ $testruns != 0 ]; do
   qstat -j $stop_job_id 
   tstate=`echo $?`
   if [ $tstate -eq 0 ]; then
      echo "job id $stop_job_id allready running."
      exit 0
   fi
   
   qacct -j $stop_job_id 
   tstate=`echo $?`
   if [ $tstate -eq 0 ]; then
      echo "job id $stop_job_id allready done."
      exit 0
   fi
   testruns=`expr $testruns - 1`
done


while [ $do_submit != 0 ]; do
   for i in $times ; do
      if [ $act_job_id -lt $stop_job_id ]; then
          echo "S`date +\|20%y.%m.%d\|%H:%M:%S\|`$hostname|last job id: $act_job_id" >> $logfile
          act_job_id=`qsub -e /dev/null -o /dev/null $jobscript $i $touchdir | tee -a $logfile | cut -f 3 -d" "`
          echo "E`date +\|20%y.%m.%d\|%H:%M:%S\|` qsub done.\n" >> $logfile
          echo "submitted job $act_job_id."
          submitted=`expr $submitted + 1`
          echo $submitted >> $jobs_submitted_file
       else
          do_submit=0    
          break 
       fi
   done
done

