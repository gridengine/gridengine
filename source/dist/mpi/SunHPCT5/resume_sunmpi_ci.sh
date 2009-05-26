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
# Resume an MPI job when its parent batch job is resumed
#
# It assumes that CRE JID is saved by suspend method.
#
#----------------
# Create a log file for tracing/debugging purposes.
#

tmpdir=$TMPDIR/resume_sunmpi_ci.$1
mkdir -p $tmpdir
cd $tmpdir

# create log file
#
F=$tmpdir/resume_sunmpi_ci.log
touch $F

me=`basename $0`
if [ $# -ne 2 ]; then
   echo "$me: got wrong number of arguments" >> $F 2>&1
   exit 1
fi

#
# Pass SGE $job_pid.
job_pid=$1
# Pass SGE $job_id.
job_id=$2

echo -------------------------------------------------------------  >> $F 2>&1
echo `basename $0` called at `date`      >> $F 2>&1
echo called by: `id`                    >> $F 2>&1
echo with args: $*                      >> $F 2>&1

#
# Resume SGE job, $job_pid.
#
/usr/bin/kill -CONT -$job_pid
#
# Determine whether it is a serial or parallel job
#
if [ X"$PE" != X ]; then
   #
   # This is a parallel job since PE is defined.
   #
   if [ -s $TMPDIR/mpijob ]; then
      jobname=`cat $TMPDIR/resume_cre_jobname`
      echo "Sending CONT signal to MPI job:$jobname"  >> $F 2>&1
      /opt/SUNWhpc/bin/mpkill -CONT $jobname >> $F 2>&1
      #
      # Delete the CRE job id file
      #
      /bin/rm $TMPDIR/mpijob
      /bin/rm $TMPDIR/resume_cre_jobname
   fi
fi
#
# signal success to caller
#
exit 0
