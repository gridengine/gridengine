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

# args: root-dir, number of loops, job types, id

if [ $# -ne 5 ]; then
   echo wrong number of args
   exit 1
fi

root_dir=$1
cell=$2
loops=$3
job_types=$4
submitter_id=$5

. $root_dir/$cell/common/settings.sh

ARCH=`$SGE_ROOT/util/arch`
QSUB=$SGE_ROOT/bin/$ARCH/qsub

#echo "loops       $loops"
#echo "job_types   $job_types"
#echo "submitter   $submitter_id"
#echo "ARCH        $ARCH"
#echo "QSUB        $QSUB"

x=0
i=0
error=0
while [ $i -lt $loops ]; do
   for j in $job_types; do
      $QSUB -N S_${submitter_id}_${x} -o /dev/null -j y $SGE_ROOT/examples/jobs/sleeper.sh $j >/dev/null 2>&1
      if [ $? != 0 ]; then
         error=1
      fi   
      x=`expr $x + 1`
   done
   i=`expr $i + 1`
done

echo "finished submitting"
exit $error
