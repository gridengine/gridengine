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

# -------------------------------------------
# --          use Bourne shell             --
#$ -S /bin/sh
# -------------------------------------------
# --     What to redirect to where         --

if [ x$SGE_ROOT = x ]; then
   echo "no SGE_ROOT found"
   exit 1
fi

if [ x$ARC = x ]; then
   ARC=`$SGE_ROOT/util/arch`
fi   

QSUB=$SGE_ROOT/bin/$ARC/qsub

echo using $QSUB as qsub command

if [ x$1 = x ]; then
   DIRECTORY=`pwd`
else
   DIRECTORY=$1
fi

if [ x$2 = x ]; then
   prefix=x
else
   prefix=$2
fi

if [ x$3 = x ]; then
   num=1
else
   num=`expr $3 + 1`
fi

if [ x$4 = x ]; then
   SLEEP=120
else
   SLEEP=$4
fi   

NAME=W_${prefix}_${num}


# started by SGE or manually 
if [ x$JOB_ID = x ]; then
   echo "submitting $NAME"
else
   sleep $SLEEP
fi

# first try
cmd="$QSUB -v COMMD_PORT=$COMMD_PORT -o /dev/null -j y -N $NAME $DIRECTORY/pminiworm.sh $DIRECTORY $prefix $num $SLEEP"
$cmd

# repeat until success 
while [ "x$?" != "x0" ]; do
   echo "pminiworm.sh: qsub failed - retrying .." >&2
   sleep $SLEEP
   $cmd
done
