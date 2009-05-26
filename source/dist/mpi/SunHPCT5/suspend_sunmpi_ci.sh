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
#
# Suspend an MPI job when its parent batch job is suspended
#

# Create a log file for tracing/debugging purposes.
#
tmpdir=$TMPDIR/suspend_sunmpi_ci.$1
mkdir -p $tmpdir
cd $tmpdir

# create log file
#
F=$tmpdir/suspend_sunmpi_ci.log
touch $F

# Pass SGE $job_pid & $job_id.
me=`basename $0`
if [ $# -ne 2 ]; then
   echo "$me: got wrong number of arguments" >> $F 2>&1
   exit 1
fi
job_pid=$1
job_id=$2

echo -------------------------------------------------------------  >> $F 2>&1
echo `basename $0` called at `date`      >> $F 2>&1
echo called by: `id`                    >> $F 2>&1
echo with args: $*                      >> $F 2>&1

#
# Stop the process of SGE job, $job_pid.
#
/usr/bin/kill -STOP -$job_pid >> $F 2>&1
#
# Determine whether it is a serial or parallel job
#
if [ X"$PE" != X ]; then
   #
   # This is a parallel job since PE is defined.
   #
   if [ -s /opt/SUNWhpc/bin/mpinfo ]; then
      verString=`/opt/SUNWhpc/bin/mpinfo -V 2>&1`
      isClusterTools=`echo $verString | grep ClusterTools`
      isCT4=`echo $verString | grep "HPC ClusterTools 4"`
      if [ X"$isClusterTools" != X ]; then
         #
         # This version is Sun HPC ClusterTools 4 or later 
         #
         if [ X"$isCT4" != X ]; then
            #
            # This version is Sun HPC ClusterTools 4.
            #
            echo "This cluster is running Sun HPC ClusterTools 4 software." >> $F 2>&1
            echo "The suspend method works only for Sun HPC ClusterTools 5 or later version." >> $F 2>&1
            exit 1
         else
            #
            # Found Sun HPC ClusterTools 5 or later version
            # Check the current parallel job is a Sun MPI job of CT 5
            # Assuming that this cluster configured a close integration.
      	    #
            jobname="sge.$job_id"
	    isFound=`/opt/SUNWhpc/bin/mpps -e -J jid | grep $jobname`
  	    if [ X"$isFound" != X ]; then
	       #
	       # Found Sun MPI job
               # Delegate SIGSTOP to MPI processes.
               #
               /usr/bin/echo $jobname > $TMPDIR/resume_cre_jobname
               /usr/bin/echo "This is a Sun MPI job." > $TMPDIR/mpijob
	       echo "Sending STOP signal to MPI job: $jobname" >> $F 2>&1
               /opt/SUNWhpc/bin/mpkill -STOP $jobname >> $F 2>&1
            fi 
         fi
      else
         #
         # This version is Sun HPC 3.1 or earlier 
         #
         echo "This cluster is running Sun HPC 3.1 or earlier software." >> $F 2>&1
         echo "The suspend method works only for Sun HPC ClusterTools 5 or later version." >> $F 2>&1
         exit 1
      fi
   fi
fi
#
# signal success to caller
exit 0

